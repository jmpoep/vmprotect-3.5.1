using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

// ReSharper disable once CheckNamespace
namespace VMProtect
{
	public class StringManager
	{
		public StringManager(long instance)
		{
			_instance = instance;
			var key = new byte[8];
			Marshal.Copy(new IntPtr(_instance + (uint)Faces.KEY_INFO), key, 0, key.Length);
			_cipher = new CipherRC5(key);
			_entries = new Dictionary<uint, uint>();
			_key = BitConverter.ToUInt32(key, 0);

			var startPosition = (uint)Faces.STRING_INFO;
			// DecryptDirectory
			var directory = new byte[8];
			Marshal.Copy(new IntPtr(_instance + startPosition), directory, 0, directory.Length);
			directory = _cipher.Decrypt(directory);
			var numberOfEntries = BitConverter.ToUInt32(directory, 0);

			var entry = new byte[16];
			for (uint i = 0; i < numberOfEntries; i++)
			{
				// DecryptEntry
				uint pos = startPosition + 8 + i * 16;
				Marshal.Copy(new IntPtr(_instance + pos), entry, 0, 16);
				entry = _cipher.Decrypt(entry);
				_entries.Add(BitConverter.ToUInt32(entry, 0), pos);
			}
		}

		public string DecryptString(uint stringId)
		{
			uint pos;
			if (_entries.TryGetValue(stringId, out pos))
			{
				var entry = new byte[16];
				Marshal.Copy(new IntPtr(_instance + pos), entry, 0, 16);
				entry = _cipher.Decrypt(entry);

				var size = BitConverter.ToInt32(entry, 8);
				var stringData = new byte[size];
				Marshal.Copy(new IntPtr(_instance + BitConverter.ToUInt32(entry, 4)), stringData, 0, size);
				for (var c = 0; c < size; c++)
				{
					stringData[c] = (byte)(stringData[c] ^ BitRotate.Left(_key, c) + c);
				}
				return Encoding.Unicode.GetString(stringData);
			}

			return null;
		}

		private readonly long _instance;
		private readonly CipherRC5 _cipher;
		private readonly Dictionary<uint, uint> _entries;
		private readonly uint _key;
	}
}