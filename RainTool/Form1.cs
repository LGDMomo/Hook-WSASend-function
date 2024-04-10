using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO.Pipes;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RainTool
{
    public partial class Form1 : Form
    {
        NamedPipeServerStream pipeServer;

        [DllImport("kernel32.dll")]
        public static extern IntPtr OpenProcess(int dwDesiredAccess, bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        public static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("kernel32", CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
        static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
        static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress,
            uint dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint nSize, out UIntPtr lpNumberOfBytesWritten);

        [DllImport("kernel32.dll")]
        static extern IntPtr CreateRemoteThread(IntPtr hProcess,
            IntPtr lpThreadAttributes, uint dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, IntPtr lpThreadId);

        // privileges
        const int PROCESS_CREATE_THREAD = 0x0002;
        const int PROCESS_QUERY_INFORMATION = 0x0400;
        const int PROCESS_VM_OPERATION = 0x0008;
        const int PROCESS_VM_WRITE = 0x0020;
        const int PROCESS_VM_READ = 0x0010;

        // used for memory allocation
        const uint MEM_COMMIT = 0x00001000;
        const uint MEM_RESERVE = 0x00002000;
        const uint PAGE_READWRITE = 4;

        public Form1()
        {
            InitializeComponent();
        }

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void checkBox5_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void button1_Click(object sender, EventArgs e)
        {
            Process[] processes = Process.GetProcessesByName("javaw");
            if (processes.Length == 0)
            {
                MessageBox.Show("Java process (javaw.exe) not found.");
                return;
            }

            Process targetProcess = processes[0];
            IntPtr procHandle = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, false, targetProcess.Id);
            if (procHandle == IntPtr.Zero)
            {
                MessageBox.Show("Failed to open process.");
                return;
            }

            string dllName = "C:\\Users\\Amri\\Desktop\\RainTool\\Rain.dll";
            IntPtr loadLibraryAddr = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
            if (loadLibraryAddr == IntPtr.Zero)
            {
                MessageBox.Show("Failed to get address of LoadLibraryA.");

                return;
            }

            IntPtr allocMemAddress = VirtualAllocEx(procHandle, IntPtr.Zero, (uint)((dllName.Length + 1) * Marshal.SizeOf(typeof(char))), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (allocMemAddress == IntPtr.Zero)
            {
                MessageBox.Show("Failed to allocate memory in remote process.");

                return;
            }

            UIntPtr bytesWritten;
            if (!WriteProcessMemory(procHandle, allocMemAddress, Encoding.Default.GetBytes(dllName), (uint)((dllName.Length + 1) * Marshal.SizeOf(typeof(char))), out bytesWritten))
            {
                MessageBox.Show("Failed to write to remote process memory.");

                return;
            }

            IntPtr threadHandle = CreateRemoteThread(procHandle, IntPtr.Zero, 0, loadLibraryAddr, allocMemAddress, 0, IntPtr.Zero);
            if (threadHandle == IntPtr.Zero)
            {
                MessageBox.Show("Failed to create remote thread.");

                return;
            }

            MessageBox.Show("DLL injected successfully.");

        }

        private void Form1_Load(object sender, EventArgs e)
        {
            // Start named pipe server
            pipeServer = new NamedPipeServerStream("SendDataPipe", PipeDirection.In, 1, PipeTransmissionMode.Byte, PipeOptions.Asynchronous);

            pipeServer.BeginWaitForConnection(PipeConnectedCallback, null);

        }
        private void PipeConnectedCallback(IAsyncResult ar)
        {
            // Client connected, begin reading
            pipeServer.EndWaitForConnection(ar);

            byte[] buffer = new byte[1024];
            int bytesRead = pipeServer.Read(buffer, 0, buffer.Length);

            // Convert received bytes to string and display in RichTextBox
            string data = Encoding.UTF8.GetString(buffer, 0, bytesRead);
            richTextBox1.Invoke((MethodInvoker)(() => richTextBox1.AppendText(data + Environment.NewLine)));

            // Continue listening for next message
            pipeServer.Disconnect();
            pipeServer.BeginWaitForConnection(PipeConnectedCallback, null);
        }
        public void AppendTextFromDLL(string text)
        {
            richTextBox1.Invoke((MethodInvoker)(() => richTextBox1.AppendText(text)));
        }

    }
}
