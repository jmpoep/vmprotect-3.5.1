using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;

// ReSharper disable once CheckNamespace
namespace VMProtect
{
	public class HardwareID
	{
		private const int OldMethodBlocks = 8;
		private const int MaxBlocks = 16 + OldMethodBlocks;
		private const uint TypeMask = 3;

		private enum BlockType {
			Cpu,
			Host,
			Mac,
			Hdd
		};

		private readonly int _startBlock;
		private readonly List<uint> _blocks = new List<uint>();

		public HardwareID()
		{
			// old methods
			GetCpu(0);
			GetCpu(1);
			_startBlock = _blocks.Count;
			// new methods, we'll return HWID starting from this DWORD
			GetCpu(2); 
			GetMachineName();
			GetHdd();
			GetMacAddresses();
		}

		private void AddBlock(byte[] p, BlockType type)
		{
			if (_blocks.Count == MaxBlocks) return; // no free space
			if (p.Length == 0) return;

			using (var hash = new SHA1Managed())
			{
				var h = hash.ComputeHash(p);
				var block = (uint)((h[0] << 24) | (h[1] << 16) | (h[2] << 8) | h[3]);
				block &= ~TypeMask; // zero two lower bits
				block |= (uint)type & TypeMask; // set type bits

				// check existing blocks
				for (var i = _blocks.Count; i > _startBlock; i--) {
					var prevBlock = _blocks[i - 1];
					if (prevBlock == block)
						return;
					if ((prevBlock & TypeMask) != (block & TypeMask))
						break;
				}

				_blocks.Add(block);
			}
		}

		private void GetCpu(int method)
		{
			//TODO: foreach cpu:
			var info = CpuId.Invoke(1);
			if ((info[0] & 0xFF0) == 0xFE0) 
				info[0] ^= 0x20; // fix Athlon bug
			info[1] &= 0x00FFFFFF; // mask out APIC Physical ID

			if (method == 2) {
				info[2] = 0;
			} else if (method == 1) {
				info[2] &= ~(1 << 27);
			}
	
			var infob = new byte[16];
			Buffer.BlockCopy(info, 0, infob, 0, infob.Length);
			AddBlock(infob, BlockType.Cpu);
		}

		private void GetMachineName()
		{
			try
			{
				var hn = Encoding.Unicode.GetBytes(Dns.GetHostName().ToUpperInvariant());
				AddBlock(hn, BlockType.Host);
			}
			catch (SocketException)
			{
				
			}
		}

		private void ProcessMac(byte[] p)
		{
			// this big IF construction allows to put constants to the code, not to the data segment
			// it is harder to find them in the code after virtualisation
			var dw = (p[0] << 16) + (p[1] << 8) + p[2];
			if (dw == 0x000569 || dw == 0x000C29 || dw == 0x001C14 || dw == 0x005056 || // vmware
				dw == 0x0003FF || dw == 0x000D3A || dw == 0x00125A || dw == 0x00155D || dw == 0x0017FA || dw == 0x001DD8 || dw == 0x002248 || dw == 0x0025AE || dw == 0x0050F2 || // microsoft
				dw == 0x001C42 || // parallels
				dw == 0x0021F6) // virtual iron
				return;

			AddBlock(p, BlockType.Mac);
		}

		private void GetMacAddresses()
		{
			var blockCountNoMac = _blocks.Count;
			byte[] paBytes = null;
			foreach (var nic in NetworkInterface.GetAllNetworkInterfaces())
			{
				if(nic.NetworkInterfaceType != NetworkInterfaceType.Ethernet)
					continue;
				paBytes = nic.GetPhysicalAddress().GetAddressBytes();
				if (paBytes.Length >= 3)
					ProcessMac(paBytes);
			}
			if (blockCountNoMac == _blocks.Count && paBytes != null && paBytes.Length >= 3)
				AddBlock(paBytes, BlockType.Mac);
		}

		private void GetHdd()
		{
			try
			{
				switch (Environment.OSVersion.Platform)
				{
					case PlatformID.MacOSX:
						//TODO
						/*
						DASessionRef session = DASessionCreate(NULL);
						if (session) {
							struct statfs statFS;
							statfs ("/", &statFS);
							DADiskRef disk = DADiskCreateFromBSDName(NULL, session, statFS.f_mntfromname);
							if (disk) {
								CFDictionaryRef descDict = DADiskCopyDescription(disk);
								if (descDict) {
									CFUUIDRef value = (CFUUIDRef)CFDictionaryGetValue(descDict, CFSTR("DAVolumeUUID"));
									CFUUIDBytes bytes = CFUUIDGetUUIDBytes(value);
									AddBlock(&bytes, sizeof(bytes), BLOCK_HDD);
									CFRelease(descDict);
								}
								CFRelease(disk);
							}
							CFRelease(session);
						}
						*/
						break;
					case PlatformID.Unix:
						//TODO: нет ли здесь лишних дисков?
						var rootUuids = new DirectoryInfo ("/dev/disk/by-uuid");
						var uuids = new StringBuilder();
						foreach (var f in rootUuids.GetFiles("*"))
						{
							uuids.Append(f.Name);
						}
						if(uuids.Length > 0)
							AddBlock(Encoding.UTF8.GetBytes(uuids.ToString()), BlockType.Hdd);
						break;
					default:
						/*
						 * mono for windows: unimplemented
						 * 
						 * var moType = Assembly.Load("System.Management, Version=2.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a").
							GetType("System.Management.ManagementObject");
						var disk = Activator.CreateInstance(moType, 
							BindingFlags.CreateInstance, null, 
							new object[] { string.Format("Win32_LogicalDisk.DeviceID='{0}:'", Environment.SystemDirectory[0]) }, 
							CultureInfo.InvariantCulture);
						var props = moType.InvokeMember("Properties", BindingFlags.GetProperty, null, disk, null);
						var sn = props.GetType().GetMethod("get_Item").Invoke(props, new object[] { "volumeSerialNumber" });
						var snv = sn.GetType().GetMethod("get_Value").Invoke(sn, null).ToString();
						Console.WriteLine("GetHdd volumeSerialNumber = {0}", snv);
						var bytes = BitConverter.GetBytes(Convert.ToUInt32(snv, 16));*/
					
						var driveLetter = Path.GetPathRoot(Environment.SystemDirectory);
						uint serialNumber = 0;
						uint maxComponentLength = 0, fileSystemFlags = 0;
						if (Win32.GetVolumeInformation(driveLetter, null, 
							0, ref serialNumber, ref maxComponentLength,
							ref fileSystemFlags, null, 0) && serialNumber != 0)
						{
							AddBlock(BitConverter.GetBytes(serialNumber), BlockType.Hdd);
						}
						break;
				}
			}
			catch (Exception)
			{
				// ignored
			}
		}

		public byte[] GetBytes()
		{
			var ms = new MemoryStream();
			for (var i = _startBlock; i < _blocks.Count; i++)
			{
				ms.Write(BitConverter.GetBytes(_blocks[i]), 0, 4);
			}
			return ms.ToArray();
		}

		public override string ToString()
		{
			return Convert.ToBase64String(GetBytes());
		}

		public bool IsCorrect(byte[] p)
		{
			if (p.Length == 0 || (p.Length & 3) != 0) 
				return false;

			var equals = new bool[4];
			var found = new bool[4];

			foreach (var id1 in _blocks)
			{
				found[id1 & 3] = true;
				for (var i = 0; i < p.Length; i += 4) {
					var id2 = BitConverter.ToUInt32(p, i);
					if (id1 == id2) {
						equals[id1 & 3] = true;
						break;
					}
				}
			}

			// 1. check CPU
			if (!equals[0])
				return false;

			// 2. check if at least 2 of 3 items are OK
			var n = 0;
			var c = 0;
			for (var i = 0; i < 4; i++) {
				if (found[i])
					c++;
				if (equals[i])
					n++;
			}
			return n == c || n >= 3;
		}

		/*public bool IsCorrect(CryptoContainer &cont, size_t offset, size_t size)
		{
			if (size == 0 || (size & 3) || size > MAX_BLOCKS * sizeof(uint32_t)) 
				return false;

			uint32_t buff[MAX_BLOCKS];
			for (size_t i = 0; i < size / sizeof(uint32_t); i++) {
				buff[i] = cont.GetDWord(offset + i * sizeof(uint32_t));
			}
			return IsCorrect(reinterpret_cast<uint8_t *>(buff), size);
		}*/
	}
}