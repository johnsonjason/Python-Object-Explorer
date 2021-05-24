using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.IO;

namespace PyperLoader
{
    public partial class Form1 : Form
    {
        IPEndPoint localEndPoint;
        Socket PyperSocket;
        bool Debug = false;
        public Form1()
        {
            InitializeComponent();
        }
        int ExecuteClient()
        {
            localEndPoint = new IPEndPoint(IPAddress.Parse("127.0.0.1"), 8787);

            // Creation TCP/IP Socket using 
            // Socket Class Costructor
            PyperSocket = new Socket(AddressFamily.InterNetwork,
                       SocketType.Stream, ProtocolType.Tcp);

            try
            {
                PyperSocket.Connect(localEndPoint);
                PyperSocket.ReceiveTimeout = 1000;
            }
            catch (Exception e)
            {

                return 0;
            }

            // We print EndPoint information 
            // that we are connected
            textBox2.Text += "Socket connected to -> " + PyperSocket.RemoteEndPoint.ToString() + "\n";
            return 1;
        }
        private void Form1_Load(object sender, EventArgs e)
        {
             int ClientStatus = ExecuteClient();
             while (ClientStatus == 0)
             {
                 ClientStatus = ExecuteClient();
             }
             treeView1.Nodes.Add("Python Globals");
             treeView1.Nodes.Add("Python Modules");
            treeView1.Nodes.Add("Debug Context");
        }

        private void button1_Click(object sender, EventArgs e)
        {
            string AddrObj = new string(treeView1.SelectedNode.Text.SkipWhile(c => c != '[')
                                   .Skip(1)
                                   .TakeWhile(c => c != ']')
                                   .ToArray()).Trim().Remove(0, 2);

            string[] QueryStr = textBox1.Text.Split(',');
            if (QueryStr.Length != 2)
            {
                return;
            }

            string NodeQuery = "MODVALSWAP";

            NodeQuery += "|" + AddrObj + "|" + QueryStr[0] + "|" + QueryStr[1];
            PyperSocket.Send(Encoding.ASCII.GetBytes(NodeQuery + "\0"));

            try
            {
                byte[] messageReceived = new byte[65535];
                int bytesRecv = PyperSocket.Receive(messageReceived);
                textBox2.AppendText("\r\n" + Encoding.ASCII.GetString(messageReceived, 0, bytesRecv));

            }
            catch (Exception RecvEx)
            {
                return;
            }
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {

        }

        private void treeView1_AfterSelect_1(object sender, TreeViewEventArgs e)
        {

            if (e.Node.Text == "Python Modules")
            {
                PyperSocket.Send(Encoding.ASCII.GetBytes("_moduletree_\0"));
            }
            else if (e.Node.Text == "Python Globals")
            {
                PyperSocket.Send(Encoding.ASCII.GetBytes("_globaltree_\0"));
            }
            else if (e.Node.Text.Contains("Debug"))
            {
                if (!Debug)
                {
                    e.Node.Nodes.Clear();
                }
                return;
            }
            else if (e.Node.Text.Contains('[') && Debug == true)
            {
                try
                {
                    string AddrObj = new string(treeView1.SelectedNode.Text.SkipWhile(c => c != '[')
                               .Skip(1)
                               .TakeWhile(c => c != ']')
                               .ToArray()).Trim().Remove(0, 2);
                    Clipboard.SetText(AddrObj);
                    string Sender;
                    if (Debug == true)
                    {
                        Sender = "enum|" + AddrObj + "\0";
                    }
                    else
                    {
                        return;
                    }
                    PyperSocket.Send(Encoding.ASCII.GetBytes(Sender));
                    byte[] messageReceived = new byte[200000];
                    int bytesRecv = PyperSocket.Receive(messageReceived);

                    string[] TreeNodes = Encoding.ASCII.GetString(messageReceived, 0, bytesRecv).Split("|");
                    Array.Sort(TreeNodes);
                    e.Node.Nodes.Clear();
                    for (int i = 0; i < TreeNodes.Length; i++)
                    {
                        if (TreeNodes[i] == "")
                        {
                            continue;
                        }
                        e.Node.Nodes.Add(TreeNodes[i]);
                    }
                }
                catch (Exception AddrE)
                {
                    return;
                }
                return;
            }
            else
            {
                List<string> SendNodes = new List<string>();

                TreeNode CurrentNode = e.Node;
                string NodeQuery = "";
                string ModuleBuilder = "";

                while (CurrentNode.Parent != null)
                {
                    if (CurrentNode.Parent.Text == "Python Modules" || CurrentNode.Parent.Text == "Python Globals")
                    {
                        if (CurrentNode.Parent.Text == "Python Modules")
                        {
                            NodeQuery += "TREE_MOD|";
                            ModuleBuilder = CurrentNode.Text.Substring(0, CurrentNode.Text.IndexOf('\x20'));
                        }
                        else if (CurrentNode.Parent.Text == "Python Globals")
                        {
                            ModuleBuilder = CurrentNode.Text.Substring(0, CurrentNode.Text.IndexOf('\x20'));
                            NodeQuery += "TREE_GLOBAL|";
                        }
                        break;
                    }
                    string notSafe = new string(treeView1.SelectedNode.Text.SkipWhile(c => c != '(')
                           .Skip(1)
                           .TakeWhile(c => c != ')')
                           .ToArray()).Trim();
                    if (notSafe == "Unknown")
                    {
                        textBox2.AppendText("\r\nNot safe to view those elements.");
                        return;
                    }
                    SendNodes.Add(CurrentNode.Text.Substring(0, CurrentNode.Text.IndexOf('\x20')));
                    CurrentNode = CurrentNode.Parent;
                }

                NodeQuery += ModuleBuilder + "|";

                string NodeQueryLoop = "";
                if (SendNodes.Count() > 0)
                {
                    for (int i = SendNodes.Count - 1; i >= 0; i--)
                    {
                        NodeQueryLoop += SendNodes[i] + ".";
                    }
                }
                else
                {
                    NodeQueryLoop += "_NONE_";
                }

                StringBuilder sb = new StringBuilder(NodeQueryLoop);
                int place = NodeQueryLoop.LastIndexOf('.');
                if (place > -1)
                {
                    sb[place] = '|';
                }
                NodeQueryLoop = sb.ToString();

                NodeQuery += NodeQueryLoop;

                if (SendNodes.Count() > 0)
                {
                    NodeQuery += SendNodes[0] + "\0";
                }
                else
                {
                    NodeQuery += "|_NONE_\0";
                }
                PyperSocket.Send(Encoding.ASCII.GetBytes(NodeQuery));
            }

            try
            {
                byte[] messageReceived = new byte[200000];
                int bytesRecv = PyperSocket.Receive(messageReceived);
                string[] TreeNodes = Encoding.ASCII.GetString(messageReceived, 0, bytesRecv).Split("|");
                Array.Sort(TreeNodes);
                e.Node.Nodes.Clear();
                for (int i = 0; i < TreeNodes.Length; i++)
                {
                    if (TreeNodes[i] == "")
                    {
                        continue;
                    }
                    e.Node.Nodes.Add(TreeNodes[i]);
                }
            }
            catch (Exception RecvEx)
            {
                return;
            }
        }

        private void panel2_Paint(object sender, PaintEventArgs e)
        {

        }

        private void label2_Click(object sender, EventArgs e)
        {

        }

        private void button2_Click(object sender, EventArgs e)
        {

            if (Debug)
            {
                TreeNode DebugNode = treeView1.SelectedNode;
                if (DebugNode.Text.Contains('['))
                {
                    try
                    {
                        string AddrObj = new string(DebugNode.Text.SkipWhile(c => c != '[')
                                   .Skip(1)
                                   .TakeWhile(c => c != ']')
                                   .ToArray()).Trim().Remove(0, 2);

                        var Operate = DebugNode.Text.AsSpan();
                        var startPoint = Operate.IndexOf('(') + 1;
                        var length = Operate.LastIndexOf(')') - startPoint;
                        var output = Operate.Slice(startPoint, length);


                        Clipboard.SetText(AddrObj);

                        string Sender = "__getval__|" + AddrObj + "|" + output.ToString() + "\0";
                        PyperSocket.Send(Encoding.ASCII.GetBytes(Sender));
                        byte[] messageReceived = new byte[200000];
                        int bytesRecv = PyperSocket.Receive(messageReceived);
                        textBox2.AppendText("\r\n" + Encoding.ASCII.GetString(messageReceived, 0, bytesRecv));
                    }
                    catch (Exception AddrE)
                    {
                        return;
                    }
                    return;
                }
                return;
            }

            List<string> SendNodes = new List<string>();

            TreeNode CurrentNode = treeView1.SelectedNode;
            string NodeQuery = "";
            string ModuleBuilder = "";

            while (CurrentNode.Parent != null)
            {
                if (CurrentNode.Parent.Text == "Python Modules" || CurrentNode.Parent.Text == "Python Globals")
                {
                    if (CurrentNode.Parent.Text == "Python Modules")
                    {
                        NodeQuery += "MODVAL|";
                        ModuleBuilder = CurrentNode.Text.Substring(0, CurrentNode.Text.IndexOf('\x20'));
                    }
                    else if (CurrentNode.Parent.Text == "Python Globals")
                    {
                        ModuleBuilder = CurrentNode.Text.Substring(0, CurrentNode.Text.IndexOf('\x20'));
                        NodeQuery += "GLOBALVAL|";
                    }
                    break;
                }
                SendNodes.Add(CurrentNode.Text.Substring(0, CurrentNode.Text.IndexOf('\x20')));
                CurrentNode = CurrentNode.Parent;
            }

            NodeQuery += ModuleBuilder + "|";

            string NodeQueryLoop = "";
            if (SendNodes.Count() > 0)
            {
                for (int i = SendNodes.Count - 1; i >= 0; i--)
                {
                    NodeQueryLoop += SendNodes[i] + ".";
                }
            }
            else
            {
                NodeQueryLoop += "_NONE_";
            }

            StringBuilder sb = new StringBuilder(NodeQueryLoop);
            int place = NodeQueryLoop.LastIndexOf('.');
            if (place > -1)
            {
                sb[place] = '|';
            }
            NodeQueryLoop = sb.ToString();

            NodeQuery += NodeQueryLoop;

            if (SendNodes.Count() > 0)
            {
                NodeQuery += SendNodes[0];
            }
            else
            {
                NodeQuery += "|_NONE_";
            }
            NodeQuery += "|" + new string(treeView1.SelectedNode.Text.SkipWhile(c => c != '(')
                           .Skip(1)
                           .TakeWhile(c => c != ')')
                           .ToArray()).Trim();
            PyperSocket.Send(Encoding.ASCII.GetBytes(NodeQuery + "\0"));

            try
            {
                byte[] messageReceived = new byte[65535];
                int bytesRecv = PyperSocket.Receive(messageReceived);
                textBox2.AppendText("\r\n" + Encoding.ASCII.GetString(messageReceived, 0, bytesRecv));
                
            }
            catch (Exception RecvEx)
            {
                return;
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            string Sender = "querystr|" + textBox1.Text + "\0";
            PyperSocket.Send(Encoding.ASCII.GetBytes(Sender));
        }

        private void button5_Click(object sender, EventArgs e)
        {
            if (Debug == true)
            {
                PyperSocket.Send(Encoding.ASCII.GetBytes("debug|none\0"));
                Debug = false;
                return;
            }
            PyperSocket.Send(Encoding.ASCII.GetBytes("debug|none\0"));
            Debug = true;
            try
            {
                byte[] messageReceived = new byte[200000];
                int bytesRecv = PyperSocket.Receive(messageReceived);

                string[] TreeNodes = Encoding.ASCII.GetString(messageReceived, 0, bytesRecv).Split("|");
                Array.Sort(TreeNodes);
                treeView1.Nodes[2].Nodes.Clear();
                for (int i = 0; i < TreeNodes.Length; i++)
                {
                    if (TreeNodes[i] == "")
                    {
                        continue;
                    }
                    treeView1.Nodes[2].Nodes.Add(TreeNodes[i]);
                }
            }
            catch (Exception DebugE)
            {
                Debug = false;
                PyperSocket.Send(Encoding.ASCII.GetBytes("debug|none\0"));
                return;
            }
            return;
        }

        private void button4_Click(object sender, EventArgs e)
        {
            string Sender = "_audit_|" + textBox1.Text + "\0";
            PyperSocket.Send(Encoding.ASCII.GetBytes(Sender));
        }

        private void button6_Click(object sender, EventArgs e)
        {
            if (Debug)
            {
                TreeNode DebugNode = treeView1.SelectedNode;
                if (DebugNode.Text.Contains('['))
                {
                    try
                    {
                        string AddrObj = new string(DebugNode.Text.SkipWhile(c => c != '[')
                                   .Skip(1)
                                   .TakeWhile(c => c != ']')
                                   .ToArray()).Trim().Remove(0, 2);

                        var Operate = DebugNode.Text.AsSpan();
                        var startPoint = Operate.IndexOf('(') + 1;
                        var length = Operate.LastIndexOf(')') - startPoint;
                        var output = Operate.Slice(startPoint, length);


                        Clipboard.SetText(AddrObj);

                        string Sender = "__setval__|" + AddrObj + "|" + output.ToString() + "\0";
                        PyperSocket.Send(Encoding.ASCII.GetBytes(Sender));
                        byte[] messageReceived = new byte[200000];
                        int bytesRecv = PyperSocket.Receive(messageReceived);
                        textBox2.AppendText("\r\n" + Encoding.ASCII.GetString(messageReceived, 0, bytesRecv));
                    }
                    catch (Exception AddrE)
                    {
                        return;
                    }
                    return;
                }
                return;
            }

            List<string> SendNodes = new List<string>();

            TreeNode CurrentNode = treeView1.SelectedNode;
            string NodeQuery = "";
            string ModuleBuilder = "";

            while (CurrentNode.Parent != null)
            {
                if (CurrentNode.Parent.Text == "Python Modules" || CurrentNode.Parent.Text == "Python Globals")
                {
                    if (CurrentNode.Parent.Text == "Python Modules")
                    {
                        NodeQuery += "MODVALSET|";
                        ModuleBuilder = CurrentNode.Text.Substring(0, CurrentNode.Text.IndexOf('\x20'));
                    }
                    else if (CurrentNode.Parent.Text == "Python Globals")
                    {
                        ModuleBuilder = CurrentNode.Text.Substring(0, CurrentNode.Text.IndexOf('\x20'));
                        NodeQuery += "GLOBALVALSET|";
                    }
                    break;
                }
                SendNodes.Add(CurrentNode.Text.Substring(0, CurrentNode.Text.IndexOf('\x20')));
                CurrentNode = CurrentNode.Parent;
            }

            NodeQuery += ModuleBuilder + "|";

            string NodeQueryLoop = "";
            if (SendNodes.Count() > 0)
            {
                for (int i = SendNodes.Count - 1; i >= 0; i--)
                {
                    NodeQueryLoop += SendNodes[i] + ".";
                }
            }
            else
            {
                NodeQueryLoop += "_NONE_";
            }

            StringBuilder sb = new StringBuilder(NodeQueryLoop);
            int place = NodeQueryLoop.LastIndexOf('.');
            if (place > -1)
            {
                sb[place] = '|';
            }
            NodeQueryLoop = sb.ToString();

            NodeQuery += NodeQueryLoop;

            if (SendNodes.Count() > 0)
            {
                NodeQuery += SendNodes[0];
            }
            else
            {
                NodeQuery += "|_NONE_";
            }
            NodeQuery += "|" + new string(treeView1.SelectedNode.Text.SkipWhile(c => c != '(')
                           .Skip(1)
                           .TakeWhile(c => c != ')')
                           .ToArray()).Trim();
            NodeQuery += "|" + textBox1.Text;
            PyperSocket.Send(Encoding.ASCII.GetBytes(NodeQuery + "\0"));

            try
            {
                byte[] messageReceived = new byte[65535];
                int bytesRecv = PyperSocket.Receive(messageReceived);
                textBox2.AppendText("\r\n" + Encoding.ASCII.GetString(messageReceived, 0, bytesRecv));

            }
            catch (Exception RecvEx)
            {
                return;
            }
        }

        private void panel1_Paint(object sender, PaintEventArgs e)
        {

        }
    }
}
