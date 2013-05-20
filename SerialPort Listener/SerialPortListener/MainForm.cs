using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using SerialPortListener.Serial;
using System.IO;

namespace SerialPortListener
{
    public partial class MainForm : Form
    {
        SerialPortManager _spManager;
        string globMsg;
        public MainForm()
        {
            InitializeComponent();
            globMsg = "";
            UserInitialization();
        }

      

        private void UserInitialization()
        {
            _spManager = new SerialPortManager();
            SerialSettings mySerialSettings = _spManager.CurrentSerialSettings;
            serialSettingsBindingSource.DataSource = mySerialSettings;
            portNameComboBox.DataSource = mySerialSettings.PortNameCollection;
            baudRateComboBox.DataSource = mySerialSettings.BaudRateCollection;
            dataBitsComboBox.DataSource = mySerialSettings.DataBitsCollection;
            parityComboBox.DataSource = Enum.GetValues(typeof(System.IO.Ports.Parity));
            stopBitsComboBox.DataSource = Enum.GetValues(typeof(System.IO.Ports.StopBits));

            _spManager.NewSerialDataRecieved += new EventHandler<SerialDataEventArgs>(_spManager_NewSerialDataRecieved);
            this.FormClosing += new FormClosingEventHandler(MainForm_FormClosing);

            portNameComboBox.SelectedItem = "COM3";
        }

        
        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            _spManager.Dispose();   
        }

        void _spManager_NewSerialDataRecieved(object sender, SerialDataEventArgs e)
        {
            if (this.InvokeRequired)
            {
                // Using this.Invoke causes deadlock when closing serial port, and BeginInvoke is good practice anyway.
                this.BeginInvoke(new EventHandler<SerialDataEventArgs>(_spManager_NewSerialDataRecieved), new object[] { sender, e });
                return;
            }

//           int maxTextLength = 1000; // maximum text length in text box
//           if (tbData.TextLength > maxTextLength)
//           tbData.Text = tbData.Text.Remove(0, tbData.TextLength - maxTextLength);


            // This application is connected to a GPS sending ASCCI characters, so data is converted to text

            string str = Encoding.ASCII.GetString(e.Data);
            


            tbData.AppendText(str);
            
             
            tbStats.AppendText("\r\nRecieved: " + str.Length + "[" + str + "]");
   //         tbStats.AppendText("Test: " + e.Data);
            tbData.ScrollToCaret();

        }

        // Handles the "Start Listening"-buttom click event
        private void btnStart_Click(object sender, EventArgs e)
        {
            _spManager.StartListening();
        }

        // Handles the "Stop Listening"-buttom click event
        private void btnStop_Click(object sender, EventArgs e)
        {
            _spManager.StopListening();
        }

        private void label1_Click(object sender, EventArgs e)
        {

        }

        private void button1_Click(object sender, EventArgs e)
        {
          
            string msg = "101";
            msg += tbInput.Text;
            msg += "1";
            globMsg = msg;


            _spManager.Send(tbInput.Text);
            tbStats.AppendText("\r\nSent: " + tbInput.Text.Length);
           
        }

        private void tbInput_TextChanged(object sender, EventArgs e)
        {

        }

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void wszystkie_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void button2_Click(object sender, EventArgs e)
        {
            string msg = "100";
            if (cbOrange.Checked)
                msg += "1";
            else msg += "0";

            if (cbRed.Checked)
                msg += "1";
            else msg += "0";

            if (cbBlue.Checked)
                msg += "1";
            else msg += "0";

            msg += "1";

            _spManager.Send(msg);
            tbStats.AppendText("\r\nSent: " + tbInput.Text.Length);
        }

        private void tbStats_TextChanged(object sender, EventArgs e)
        {

        }
    }
}
