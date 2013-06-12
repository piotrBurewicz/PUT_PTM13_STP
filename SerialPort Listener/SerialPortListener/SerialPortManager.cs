using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO.Ports;
using System.Reflection;
using System.ComponentModel;
using System.Threading;
using System.IO;

namespace SerialPortListener.Serial
{
    /// <summary>
    /// Manager for serial port data
    /// </summary>
    public class SerialPortManager : IDisposable
    {
        public SerialPortManager()
        {
            // Finding installed serial ports on hardware
            _currentSerialSettings.PortNameCollection = SerialPort.GetPortNames();
            _currentSerialSettings.PropertyChanged += new System.ComponentModel.PropertyChangedEventHandler(_currentSerialSettings_PropertyChanged);

            // If serial ports is found, we select the first found
            if (_currentSerialSettings.PortNameCollection.Length > 0)
                _currentSerialSettings.PortName = _currentSerialSettings.PortNameCollection[0];
        }

        
        ~SerialPortManager()
        {
            Dispose(false);
        }


        #region Fields
        private SerialPort _serialPort;
        private SerialSettings _currentSerialSettings = new SerialSettings();
        private string _latestRecieved = String.Empty;
        public event EventHandler<SerialDataEventArgs> NewSerialDataRecieved; 

        #endregion

        #region Properties
        /// <summary>
        /// Gets or sets the current serial port settings
        /// </summary>
        public SerialSettings CurrentSerialSettings
        {
            get { return _currentSerialSettings; }
            set { _currentSerialSettings = value; }
        }

        #endregion

        #region Event handlers

        void _currentSerialSettings_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            // if serial port is changed, a new baud query is issued
            if (e.PropertyName.Equals("PortName"))
                UpdateBaudRateCollection();
        }

        
        void _serialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            while (true)
            {
                if (_serialPort.IsOpen)
                {
                    int dataLength = _serialPort.BytesToRead;
                    if (_serialPort.BytesToRead >= 4)
                    {
                        byte[] data = new byte[4];
                        int nbrDataRead = _serialPort.Read(data, 0, 4);
                        if (nbrDataRead == 0)
                            return;
                        int l = ((data[2] << 8) + data[1]);

                        while (_serialPort.BytesToRead < l) ;
                        data = new byte[l];
                        nbrDataRead = _serialPort.Read(data, 0, l);
                        byte[] dane = new byte[l];
                        for (int i = 0; i < l; i++)
                        {
                            dane[i] = data[i];
                        }
                        //CRC
                        byte[] crcrec = new byte[4];
                        nbrDataRead = _serialPort.Read(data, 0, 1);
                        
                        while (_serialPort.BytesToRead < 4) ;
                        int crcodebr = _serialPort.Read(crcrec, 0, 4);
                        UInt32 aaa = Crc32.Compute(dane);
                        UInt32 bbb = BitConverter.ToUInt32(crcrec, 0); 
                 //       UInt32 ccc = BitConverter.ToUInt32(Crc32., 0);
//                        if (Crc32.Compute(dane) != BitConverter.ToUInt32(crcrec, 0)) return;
                        //CRC

                        while (_serialPort.BytesToRead < 1) ;
                        if (NewSerialDataRecieved != null)
                            NewSerialDataRecieved(this, new SerialDataEventArgs(dane));



                    }
                }
            }
            



            
            
           
        }

        #endregion

        #region Methods

        /// <summary>
        /// Connects to a serial port defined through the current settings
        /// </summary>
        public void StartListening()
        {
            // Closing serial port if it is open
            if (_serialPort != null && _serialPort.IsOpen)
                    _serialPort.Close();

            // Setting serial port settings
            _serialPort = new SerialPort(
                _currentSerialSettings.PortName,
                _currentSerialSettings.BaudRate,
                _currentSerialSettings.Parity,
                _currentSerialSettings.DataBits,
                _currentSerialSettings.StopBits);

            // Subscribe to event and open serial port for data
            _serialPort.DataReceived += new SerialDataReceivedEventHandler(_serialPort_DataReceived);
            _serialPort.Open();
        }

        /// <summary>
        /// Closes the serial port
        /// </summary>
        public void StopListening()
        {
            _serialPort.Close();
        }


        /// <summary>
        /// Retrieves the current selected device's COMMPROP structure, and extracts the dwSettableBaud property
        /// </summary>
        private void UpdateBaudRateCollection()
        {
            _serialPort = new SerialPort(_currentSerialSettings.PortName);
            _serialPort.Open();
            object p = _serialPort.BaseStream.GetType().GetField("commProp", BindingFlags.Instance | BindingFlags.NonPublic).GetValue(_serialPort.BaseStream);
            Int32 dwSettableBaud = (Int32)p.GetType().GetField("dwSettableBaud", BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public).GetValue(p);

            _serialPort.Close();
            _currentSerialSettings.UpdateBaudRateCollection(dwSettableBaud);
        }

        // Call to release serial port
        public void Dispose()
        {
            Dispose(true);
        }

        // Part of basic design pattern for implementing Dispose
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                _serialPort.DataReceived -= new SerialDataReceivedEventHandler(_serialPort_DataReceived);
            }
            // Releasing serial port (and other unmanaged objects)
            if (_serialPort != null)
            {
                if (_serialPort.IsOpen)
                    _serialPort.Close();

                _serialPort.Dispose();
            }
        }

        public void Send(string str)
        {
            //byte[] naglowek = new byte[4];
            //naglowek[0] = (byte)170;

            byte[] a = new byte[4];
            a = BitConverter.GetBytes(170);
            System.Buffer.BlockCopy(BitConverter.GetBytes(str.Length), 0, a, 1, 2);
            System.Buffer.BlockCopy(BitConverter.GetBytes(1), 0, a, 3, 1);
            _serialPort.Write(a, 0 , 4);
            _serialPort.WriteLine(str);
            //CRC
            byte[] b2 = System.Text.Encoding.ASCII.GetBytes(str);
            a = BitConverter.GetBytes(Crc32.Compute(b2));
            UInt32 aaa = Crc32.Compute(b2);     
            _serialPort.Write(a, 0, 4);
            //CRC
            a = BitConverter.GetBytes(85);
            _serialPort.Write(a, 0, 1);

        //  _serialPort.Write(str);
        //  _serialPort.Write(BitConverter.GetBytes(170), 1, 1);
        //  _serialPort.Write(BitConverter.GetBytes(170), 1, 1);
        //  _serialPort.WriteLine(str);
        }

        public void Send1(string str)
        {
            byte[] ramka = new byte[str.Length + 5];

            byte[] dl = new byte[2];
            ramka[0] = BitConverter.GetBytes(170)[0];
            dl = BitConverter.GetBytes(str.Length);
            ramka[1] = dl[0];
            ramka[2] = dl[1];
            ramka[3] = BitConverter.GetBytes(180)[0];
            byte[] tekst = new byte [str.Length];
            tekst = System.Text.Encoding.Unicode.GetBytes(str);
            for (int i = 4; i < str.Length; i++)
                ramka[i] = tekst[i-4];
            ramka[str.Length + 4] = BitConverter.GetBytes(190)[0];
         //   for(int j=0;j<str.Length;j++)
            _serialPort.Write(ramka, 0, str.Length+5);
        }

        #endregion

    }

    /// <summary>
    /// EventArgs used to send bytes recieved on serial port
    /// </summary>
    public class SerialDataEventArgs : EventArgs
    {
        public SerialDataEventArgs(byte[] dataInByteArray)
        {
            Data = dataInByteArray;
        }

        /// <summary>
        /// Byte array containing data from serial port
        /// </summary>
        public byte[] Data;
    }

    
}
