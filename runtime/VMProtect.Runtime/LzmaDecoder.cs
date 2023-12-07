// LzmaDecoder.cs

using System;
using System.IO;
using SevenZip.Compression.RangeCoder;
#pragma warning disable 414

namespace SevenZip.Compression.LZ
{
    public class OutWindow
    {
        byte[] _buffer;
        uint _pos;
        uint _windowSize;
        uint _streamPos;
        Stream _stream;
        uint _id = 1;

        public uint TrainSize;

        public void Create(uint windowSize)
        {
            if (_windowSize != windowSize)
            {
                _buffer = new byte[windowSize];
            }
            _windowSize = windowSize;
            _pos = 0;
            _streamPos = 0;
        }

        public void Init(Stream stream, bool solid)
        {
            ReleaseStream();
            _stream = stream;
            if (!solid)
            {
                _streamPos = 0;
                _pos = 0;
                TrainSize = 0;
            }
        }

        public void ReleaseStream()
        {
            Flush();
            _stream = null;
        }

        public void Flush()
        {
            uint size = _pos - _streamPos;
            if (size == 0)
                return;
            _stream.Write(_buffer, (int)_streamPos, (int)size);
            if (_pos >= _windowSize)
                _pos = 0;
            _streamPos = _pos;
        }

        public void CopyBlock(uint distance, uint len)
        {
            uint pos = _pos - distance - 1;
            if (pos >= _windowSize)
                pos += _windowSize;
            for (; len > 0; len--)
            {
                if (pos >= _windowSize)
                    pos = 0;
                _buffer[_pos++] = _buffer[pos++];
                if (_pos >= _windowSize)
                    Flush();
            }
        }

        public void PutByte(byte b)
        {
            _buffer[_pos++] = b;
            if (_pos >= _windowSize)
                Flush();
        }

        public byte GetByte(uint distance)
        {
            uint pos = _pos - distance - 1;
            if (pos >= _windowSize)
                pos += _windowSize;
            return _buffer[pos];
        }
    }
}

namespace SevenZip.Compression.RangeCoder
{
    class Decoder
    {
        uint _id = 1;

        public const uint KTopValue = (1 << 24);
        public uint Range;
        public uint Code;
        public Stream Stream;

        public void Init(Stream stream)
        {
            Stream = stream;

            Code = 0;
            Range = 0xFFFFFFFF;
            for (int i = 0; i < 5; i++)
                Code = (Code << 8) | (byte)Stream.ReadByte();
        }

        public void ReleaseStream()
        {
            Stream = null;
        }


        public uint DecodeDirectBits(int numTotalBits)
        {
            uint range = Range;
            uint code = Code;
            uint result = 0;
            for (int i = numTotalBits; i > 0; i--)
            {
                range >>= 1;
                uint t = (code - range) >> 31;
                code -= range & (t - 1);
                result = (result << 1) | (1 - t);

                if (range < KTopValue)
                {
                    code = (code << 8) | (byte)Stream.ReadByte();
                    range <<= 8;
                }
            }
            Range = range;
            Code = code;
            return result;
        }
    }

    struct BitDecoder
    {
        private const int KNumBitModelTotalBits = 11;
        private const uint KBitModelTotal = (1 << KNumBitModelTotalBits);
        const int KNumMoveBits = 5;

        uint _prob;

        public void Init() { _prob = KBitModelTotal >> 1; }

        public uint Decode(Decoder rangeDecoder)
        {
            uint newBound = (rangeDecoder.Range >> KNumBitModelTotalBits) * _prob;
            if (rangeDecoder.Code < newBound)
            {
                rangeDecoder.Range = newBound;
                _prob += (KBitModelTotal - _prob) >> KNumMoveBits;
                if (rangeDecoder.Range < Decoder.KTopValue)
                {
                    rangeDecoder.Code = (rangeDecoder.Code << 8) | (byte)rangeDecoder.Stream.ReadByte();
                    rangeDecoder.Range <<= 8;
                }
                return 0;
            }
            else
            {
                rangeDecoder.Range -= newBound;
                rangeDecoder.Code -= newBound;
                _prob -= (_prob) >> KNumMoveBits;
                if (rangeDecoder.Range < Decoder.KTopValue)
                {
                    rangeDecoder.Code = (rangeDecoder.Code << 8) | (byte)rangeDecoder.Stream.ReadByte();
                    rangeDecoder.Range <<= 8;
                }
                return 1;
            }
        }
    }

    struct BitTreeDecoder
    {
        private readonly BitDecoder[] _models;
        private readonly int _numBitLevels;

        public BitTreeDecoder(int numBitLevels)
        {
            _numBitLevels = numBitLevels;
            _models = new BitDecoder[1 << numBitLevels];
        }

        public void Init()
        {
            for (uint i = 1; i < (1 << _numBitLevels); i++)
                _models[i].Init();
        }

        public uint Decode(Decoder rangeDecoder)
        {
            uint m = 1;
            for (int bitIndex = _numBitLevels; bitIndex > 0; bitIndex--)
                m = (m << 1) + _models[m].Decode(rangeDecoder);
            return m - ((uint)1 << _numBitLevels);
        }

        public uint ReverseDecode(Decoder rangeDecoder)
        {
            uint m = 1;
            uint symbol = 0;
            for (int bitIndex = 0; bitIndex < _numBitLevels; bitIndex++)
            {
                uint bit = _models[m].Decode(rangeDecoder);
                m <<= 1;
                m += bit;
                symbol |= (bit << bitIndex);
            }
            return symbol;
        }

        public static uint ReverseDecode(BitDecoder[] models, UInt32 startIndex,
            Decoder rangeDecoder, int numBitLevels)
        {
            uint m = 1;
            uint symbol = 0;
            for (int bitIndex = 0; bitIndex < numBitLevels; bitIndex++)
            {
                uint bit = models[startIndex + m].Decode(rangeDecoder);
                m <<= 1;
                m += bit;
                symbol |= (bit << bitIndex);
            }
            return symbol;
        }
    }
}

namespace SevenZip.Compression.LZMA
{
    internal abstract class Base
    {
        public const uint KNumStates = 12;

        public struct State
        {
            public uint Index;
            public void Init() { Index = 0; }
            public void UpdateChar()
            {
                if (Index < 4) Index = 0;
                else if (Index < 10) Index -= 3;
                else Index -= 6;
            }
            public void UpdateMatch() { Index = (uint)(Index < 7 ? 7 : 10); }
            public void UpdateRep() { Index = (uint)(Index < 7 ? 8 : 11); }
            public void UpdateShortRep() { Index = (uint)(Index < 7 ? 9 : 11); }
            public bool IsCharState() { return Index < 7; }
        }

        public const int KNumPosSlotBits = 6;

        private const int KNumLenToPosStatesBits = 2; // it's for speed optimization
        public const uint KNumLenToPosStates = 1 << KNumLenToPosStatesBits;

        public const uint KMatchMinLen = 2;

        public static uint GetLenToPosState(uint len)
        {
            len -= KMatchMinLen;
            if (len < KNumLenToPosStates)
                return len;
            return KNumLenToPosStates - 1;
        }

        public const int KNumAlignBits = 4;

        public const uint KStartPosModelIndex = 4;
        public const uint KEndPosModelIndex = 14;

        public const uint KNumFullDistances = 1 << ((int)KEndPosModelIndex / 2);

        public const int KNumPosStatesBitsMax = 4;
        public const uint KNumPosStatesMax = (1 << KNumPosStatesBitsMax);

        public const int KNumLowLenBits = 3;
        public const int KNumMidLenBits = 3;
        public const int KNumHighLenBits = 8;
        public const uint KNumLowLenSymbols = 1 << KNumLowLenBits;
        public const uint KNumMidLenSymbols = 1 << KNumMidLenBits;
    }
    public class Decoder
	{
        uint _id = 1;

        class LenDecoder
		{
			BitDecoder _mChoice = new BitDecoder();
			BitDecoder _mChoice2 = new BitDecoder();
		    readonly BitTreeDecoder[] _mLowCoder = new BitTreeDecoder[Base.KNumPosStatesMax];
		    readonly BitTreeDecoder[] _mMidCoder = new BitTreeDecoder[Base.KNumPosStatesMax];
			BitTreeDecoder _mHighCoder = new BitTreeDecoder(Base.KNumHighLenBits);
			uint _mNumPosStates;

			public void Create(uint numPosStates)
			{
				for (uint posState = _mNumPosStates; posState < numPosStates; posState++)
				{
					_mLowCoder[posState] = new BitTreeDecoder(Base.KNumLowLenBits);
					_mMidCoder[posState] = new BitTreeDecoder(Base.KNumMidLenBits);
				}
				_mNumPosStates = numPosStates;
			}

			public void Init()
			{
				_mChoice.Init();
				for (uint posState = 0; posState < _mNumPosStates; posState++)
				{
					_mLowCoder[posState].Init();
					_mMidCoder[posState].Init();
				}
				_mChoice2.Init();
				_mHighCoder.Init();
			}

			public uint Decode(RangeCoder.Decoder rangeDecoder, uint posState)
			{
				if (_mChoice.Decode(rangeDecoder) == 0)
					return _mLowCoder[posState].Decode(rangeDecoder);
				else
				{
					uint symbol = Base.KNumLowLenSymbols;
					if (_mChoice2.Decode(rangeDecoder) == 0)
						symbol += _mMidCoder[posState].Decode(rangeDecoder);
					else
					{
						symbol += Base.KNumMidLenSymbols;
						symbol += _mHighCoder.Decode(rangeDecoder);
					}
					return symbol;
				}
			}
		}

		class LiteralDecoder
		{
            uint _id = 1;

            struct Decoder2
			{
				BitDecoder[] _mDecoders;
				public void Create() { _mDecoders = new BitDecoder[0x300]; }
				public void Init() { for (int i = 0; i < 0x300; i++) _mDecoders[i].Init(); }

				public byte DecodeNormal(RangeCoder.Decoder rangeDecoder)
				{
					uint symbol = 1;
					do
						symbol = (symbol << 1) | _mDecoders[symbol].Decode(rangeDecoder);
					while (symbol < 0x100);
					return (byte)symbol;
				}

				public byte DecodeWithMatchByte(RangeCoder.Decoder rangeDecoder, byte matchByte)
				{
					uint symbol = 1;
					do
					{
						uint matchBit = (uint)(matchByte >> 7) & 1;
						matchByte <<= 1;
						uint bit = _mDecoders[((1 + matchBit) << 8) + symbol].Decode(rangeDecoder);
						symbol = (symbol << 1) | bit;
						if (matchBit != bit)
						{
							while (symbol < 0x100)
								symbol = (symbol << 1) | _mDecoders[symbol].Decode(rangeDecoder);
							break;
						}
					}
					while (symbol < 0x100);
					return (byte)symbol;
				}
			}

			Decoder2[] _mCoders;
			int _mNumPrevBits;
			int _mNumPosBits;
			uint _mPosMask;

			public void Create(int numPosBits, int numPrevBits)
			{
				if (_mCoders != null && _mNumPrevBits == numPrevBits &&
					_mNumPosBits == numPosBits)
					return;
				_mNumPosBits = numPosBits;
				_mPosMask = ((uint)1 << numPosBits) - 1;
				_mNumPrevBits = numPrevBits;
				uint numStates = (uint)1 << (_mNumPrevBits + _mNumPosBits);
				_mCoders = new Decoder2[numStates];
				for (uint i = 0; i < numStates; i++)
					_mCoders[i].Create();
			}

			public void Init()
			{
				uint numStates = (uint)1 << (_mNumPrevBits + _mNumPosBits);
				for (uint i = 0; i < numStates; i++)
					_mCoders[i].Init();
			}

			uint GetState(uint pos, byte prevByte)
			{ return ((pos & _mPosMask) << _mNumPrevBits) + (uint)(prevByte >> (8 - _mNumPrevBits)); }

			public byte DecodeNormal(RangeCoder.Decoder rangeDecoder, uint pos, byte prevByte)
			{ return _mCoders[GetState(pos, prevByte)].DecodeNormal(rangeDecoder); }

			public byte DecodeWithMatchByte(RangeCoder.Decoder rangeDecoder, uint pos, byte prevByte, byte matchByte)
			{ return _mCoders[GetState(pos, prevByte)].DecodeWithMatchByte(rangeDecoder, matchByte); }
		};

        readonly LZ.OutWindow _mOutWindow = new LZ.OutWindow();
        readonly RangeCoder.Decoder _mRangeDecoder = new RangeCoder.Decoder();

        readonly BitDecoder[] _mIsMatchDecoders = new BitDecoder[Base.KNumStates << Base.KNumPosStatesBitsMax];
        readonly BitDecoder[] _mIsRepDecoders = new BitDecoder[Base.KNumStates];
        readonly BitDecoder[] _mIsRepG0Decoders = new BitDecoder[Base.KNumStates];
        readonly BitDecoder[] _mIsRepG1Decoders = new BitDecoder[Base.KNumStates];
        readonly BitDecoder[] _mIsRepG2Decoders = new BitDecoder[Base.KNumStates];
        readonly BitDecoder[] _mIsRep0LongDecoders = new BitDecoder[Base.KNumStates << Base.KNumPosStatesBitsMax];

        readonly BitTreeDecoder[] _mPosSlotDecoder = new BitTreeDecoder[Base.KNumLenToPosStates];
        readonly BitDecoder[] _mPosDecoders = new BitDecoder[Base.KNumFullDistances - Base.KEndPosModelIndex];

		BitTreeDecoder _mPosAlignDecoder = new BitTreeDecoder(Base.KNumAlignBits);

        readonly LenDecoder _mLenDecoder = new LenDecoder();
        readonly LenDecoder _mRepLenDecoder = new LenDecoder();

        readonly LiteralDecoder _mLiteralDecoder = new LiteralDecoder();

		uint _mDictionarySize;
		uint _mDictionarySizeCheck;

		uint _mPosStateMask;

		public Decoder()
		{
			_mDictionarySize = 0xFFFFFFFF;
			for (int i = 0; i < Base.KNumLenToPosStates; i++)
				_mPosSlotDecoder[i] = new BitTreeDecoder(Base.KNumPosSlotBits);
		}

		void SetDictionarySize(uint dictionarySize)
		{
			if (_mDictionarySize != dictionarySize)
			{
				_mDictionarySize = dictionarySize;
				_mDictionarySizeCheck = Math.Max(_mDictionarySize, 1);
				uint blockSize = Math.Max(_mDictionarySizeCheck, (1 << 12));
				_mOutWindow.Create(blockSize);
			}
		}

		void SetLiteralProperties(int lp, int lc)
		{
			if (lp > 8)
				throw new ArgumentException("lp > 8");
			if (lc > 8)
                throw new ArgumentException("lc > 8");
			_mLiteralDecoder.Create(lp, lc);
		}

		void SetPosBitsProperties(int pb)
		{
			if (pb > Base.KNumPosStatesBitsMax)
				throw new ArgumentException("pb > Base.KNumPosStatesBitsMax");
			uint numPosStates = (uint)1 << pb;
			_mLenDecoder.Create(numPosStates);
			_mRepLenDecoder.Create(numPosStates);
			_mPosStateMask = numPosStates - 1;
		}

		void Init(Stream inStream, Stream outStream)
		{
			_mRangeDecoder.Init(inStream);
			_mOutWindow.Init(outStream, false);

			uint i;
			for (i = 0; i < Base.KNumStates; i++)
			{
				for (uint j = 0; j <= _mPosStateMask; j++)
				{
					uint index = (i << Base.KNumPosStatesBitsMax) + j;
					_mIsMatchDecoders[index].Init();
					_mIsRep0LongDecoders[index].Init();
				}
				_mIsRepDecoders[i].Init();
				_mIsRepG0Decoders[i].Init();
				_mIsRepG1Decoders[i].Init();
				_mIsRepG2Decoders[i].Init();
			}

			_mLiteralDecoder.Init();
			for (i = 0; i < Base.KNumLenToPosStates; i++)
				_mPosSlotDecoder[i].Init();
			for (i = 0; i < Base.KNumFullDistances - Base.KEndPosModelIndex; i++)
				_mPosDecoders[i].Init();

			_mLenDecoder.Init();
			_mRepLenDecoder.Init();
			_mPosAlignDecoder.Init();
		}

		public void Code(Stream inStream, Stream outStream, Int64 outSize)
		{
			Init(inStream, outStream);

			Base.State state = new Base.State();
			state.Init();
			uint rep0 = 0, rep1 = 0, rep2 = 0, rep3 = 0;

			UInt64 nowPos64 = 0;
			UInt64 outSize64 = (UInt64)outSize;
			if (nowPos64 < outSize64)
			{
				if (_mIsMatchDecoders[state.Index << Base.KNumPosStatesBitsMax].Decode(_mRangeDecoder) != 0)
                    throw new InvalidDataException("IsMatchDecoders");
				state.UpdateChar();
				byte b = _mLiteralDecoder.DecodeNormal(_mRangeDecoder, 0, 0);
				_mOutWindow.PutByte(b);
				nowPos64++;
			}
			while (nowPos64 < outSize64)
			{
				// UInt64 next = Math.Min(nowPos64 + (1 << 18), outSize64);
					// while(nowPos64 < next)
				{
					uint posState = (uint)nowPos64 & _mPosStateMask;
					if (_mIsMatchDecoders[(state.Index << Base.KNumPosStatesBitsMax) + posState].Decode(_mRangeDecoder) == 0)
					{
						byte b;
						byte prevByte = _mOutWindow.GetByte(0);
						if (!state.IsCharState())
							b = _mLiteralDecoder.DecodeWithMatchByte(_mRangeDecoder,
								(uint)nowPos64, prevByte, _mOutWindow.GetByte(rep0));
						else
							b = _mLiteralDecoder.DecodeNormal(_mRangeDecoder, (uint)nowPos64, prevByte);
						_mOutWindow.PutByte(b);
						state.UpdateChar();
						nowPos64++;
					}
					else
					{
						uint len;
						if (_mIsRepDecoders[state.Index].Decode(_mRangeDecoder) == 1)
						{
							if (_mIsRepG0Decoders[state.Index].Decode(_mRangeDecoder) == 0)
							{
								if (_mIsRep0LongDecoders[(state.Index << Base.KNumPosStatesBitsMax) + posState].Decode(_mRangeDecoder) == 0)
								{
									state.UpdateShortRep();
									_mOutWindow.PutByte(_mOutWindow.GetByte(rep0));
									nowPos64++;
									continue;
								}
							}
							else
							{
								UInt32 distance;
								if (_mIsRepG1Decoders[state.Index].Decode(_mRangeDecoder) == 0)
								{
									distance = rep1;
								}
								else
								{
									if (_mIsRepG2Decoders[state.Index].Decode(_mRangeDecoder) == 0)
										distance = rep2;
									else
									{
										distance = rep3;
										rep3 = rep2;
									}
									rep2 = rep1;
								}
								rep1 = rep0;
								rep0 = distance;
							}
							len = _mRepLenDecoder.Decode(_mRangeDecoder, posState) + Base.KMatchMinLen;
							state.UpdateRep();
						}
						else
						{
							rep3 = rep2;
							rep2 = rep1;
							rep1 = rep0;
							len = Base.KMatchMinLen + _mLenDecoder.Decode(_mRangeDecoder, posState);
							state.UpdateMatch();
							uint posSlot = _mPosSlotDecoder[Base.GetLenToPosState(len)].Decode(_mRangeDecoder);
							if (posSlot >= Base.KStartPosModelIndex)
							{
								int numDirectBits = (int)((posSlot >> 1) - 1);
								rep0 = ((2 | (posSlot & 1)) << numDirectBits);
								if (posSlot < Base.KEndPosModelIndex)
									rep0 += BitTreeDecoder.ReverseDecode(_mPosDecoders,
											rep0 - posSlot - 1, _mRangeDecoder, numDirectBits);
								else
								{
									rep0 += (_mRangeDecoder.DecodeDirectBits(
										numDirectBits - Base.KNumAlignBits) << Base.KNumAlignBits);
									rep0 += _mPosAlignDecoder.ReverseDecode(_mRangeDecoder);
								}
							}
							else
								rep0 = posSlot;
						}
						if (rep0 >= _mOutWindow.TrainSize + nowPos64 || rep0 >= _mDictionarySizeCheck)
						{
							if (rep0 == 0xFFFFFFFF)
								break;
							throw new InvalidDataException("rep0");
						}
						_mOutWindow.CopyBlock(rep0, len);
						nowPos64 += len;
					}
				}
			}
			_mOutWindow.Flush();
			_mOutWindow.ReleaseStream();
			_mRangeDecoder.ReleaseStream();
		}

		public void SetDecoderProperties(byte[] properties)
		{
			if (properties.Length < 5)
                throw new ArgumentException("properties.Length < 5");
			int lc = properties[0] % 9;
			int remainder = properties[0] / 9;
			int lp = remainder % 5;
			int pb = remainder / 5;
			if (pb > Base.KNumPosStatesBitsMax)
                throw new ArgumentException("pb > Base.kNumPosStatesBitsMax");
			UInt32 dictionarySize = 0;
			for (int i = 0; i < 4; i++)
				dictionarySize += ((UInt32)(properties[1 + i])) << (i * 8);
			SetDictionarySize(dictionarySize);
			SetLiteralProperties(lp, lc);
			SetPosBitsProperties(pb);
		}
	}
}
