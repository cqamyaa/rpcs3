#pragma once
#include "MemoryBlock.h"

class MemoryFlags
{
	struct Flag
	{
		const u64 addr;
		const u64 waddr;
		const u64 fid;

		Flag(const u64 _addr, const u64 _waddr, const u64 _fid)
			: addr(_addr)
			, waddr(_waddr)
			, fid(_fid)
		{
		}
	};

	Array<Flag> m_memflags;

public:
	void Add(const u64 addr, const u64 waddr, const u64 fid) {m_memflags.Add(new Flag(addr, waddr, fid));}
	void Clear() { m_memflags.Clear(); }

	bool IsFlag(const u64 addr, u64& waddr, u64& fid)
	{
		for(u32 i=0; i<GetCount(); i++)
		{
			if(m_memflags[i].addr != addr) continue;
			fid = m_memflags[i].fid;
			waddr = m_memflags[i].waddr;
			return true;
		}

		return false;
	}

	u64 GetCount() const { return m_memflags.GetCount(); }
};

class MemoryBase
{
	NullMemoryBlock NullMem;
	ArrayF<MemoryBlock> MemoryBlocks;

public:
	MemoryBlock MainRam;
	MemoryBlock UserMem;
	MemoryBlock UnkMem;
	MemoryBlock VideoMem; //used by psgl
	//MemoryBlock Video_FrameBuffer;
	//MemoryBlock Video_GPUdata;

	struct UserMemInfo
	{
		u64 addr;
		u32 size;

		UserMemInfo(u64 _addr, u32 _size)
			: addr(_addr)
			, size(_size)
		{
		}
	};

	Array<UserMemInfo> m_used_usermem;
	Array<UserMemInfo> m_free_usermem;
	u32 m_mem_point;

	MemoryFlags MemFlags;

	bool m_inited;

	MemoryBase()
	{
		m_inited = false;
	}

	~MemoryBase()
	{
		Close();
	}

	static u16 Reverse16(const u16 val)
	{
		return ((val >> 8) & 0xff) | ((val << 8) & 0xff00);
	}

	static u32 Reverse32(const u32 val)
	{
		return
			((val >> 24) & 0x000000ff) |
			((val >>  8) & 0x0000ff00) |
			((val <<  8) & 0x00ff0000) |
			((val << 24) & 0xff000000);
	}

	static u64 Reverse64(const u64 val)
	{
		return
			((val >> 56) & 0x00000000000000ff) |
			((val >> 40) & 0x000000000000ff00) |
			((val >> 24) & 0x0000000000ff0000) |
			((val >>  8) & 0x00000000ff000000) |
			((val <<  8) & 0x000000ff00000000) |
			((val << 24) & 0x0000ff0000000000) |
			((val << 40) & 0x00ff000000000000) |
			((val << 56) & 0xff00000000000000);
	}

	template<typename T> static T Reverse(T val)
	{
		switch(sizeof(T))
		{
		case 2: return Reverse16(val);
		case 4: return Reverse32(val);
		case 8: return Reverse64(val);
		}

		return val;
	}

	MemoryBlock& GetMemByNum(const u8 num)
	{
		if(num >= MemoryBlocks.GetCount()) return NullMem;
		return MemoryBlocks.Get(num);
	}

	MemoryBlock& GetMemByAddr(const u64 addr)
	{
		for(uint i=0; i<MemoryBlocks.GetCount(); ++i)
		{
			if(MemoryBlocks.Get(i).IsMyAddress(addr)) return MemoryBlocks.Get(i);
		}

		return NullMem;
	}

	u8* GetMemFromAddr(const u64 addr)
	{
		return GetMemByAddr(addr).GetMemFromAddr(addr);
	}

	void Init()
	{
		if(m_inited) return;
		m_inited = true;

		ConLog.Write("Initing memory...");

		MainRam.SetRange(0x00000000, 0x10000000); //256 MB
		UnkMem. SetRange(0x10000000, 0x00100000); //unk
		UserMem.SetRange(0x2FFFFE00, 0x0D500000);
		VideoMem.SetRange(0xe0040000, 0x01000000);
		//Video_FrameBuffer.SetRange(MainRam.GetEndAddr(), 0x0FFFFFFF); //252 MB
		//Video_GPUdata.SetRange(Video_FrameBuffer.GetEndAddr(), 0x003FFFFF); //4 MB
		MemoryBlocks.Add(MainRam);
		MemoryBlocks.Add(UnkMem);
		MemoryBlocks.Add(UserMem);
		MemoryBlocks.Add(VideoMem);
		//MemoryBlocks.Add(Video_FrameBuffer);
		//MemoryBlocks.Add(Video_GPUdata);
		m_mem_point = UserMem.GetStartAddr();
	}

	bool IsGoodAddr(const u64 addr)
	{
		for(uint i=0; i<MemoryBlocks.GetCount(); ++i)
		{
			if(MemoryBlocks[i].IsMyAddress(addr)) return true;
		}

		return false;
	}

	bool IsGoodAddr(const u64 addr, const u32 size)
	{
		return IsGoodAddr(addr) && IsGoodAddr(addr + size - 1);
	}

	void Close()
	{
		if(!m_inited) return;
		m_inited = false;

		ConLog.Write("Closing memory...");

		for(uint i=0; i<MemoryBlocks.GetCount(); ++i)
		{
			MemoryBlocks[i].Delete();
		}

		MemoryBlocks.Clear();
		MemFlags.Clear();
		m_mem_point = 0;
		m_free_usermem.Clear();
		m_used_usermem.Clear();
	}

	void Reset()
	{
		if(!m_inited) return;

		ConLog.Write("Resetting memory...");
		Close();
		Init();
	}

	void Write8(const u64 addr, const u8 data);
	void Write16(const u64 addr, const u16 data);
	void Write32(const u64 addr, const u32 data);
	void Write64(const u64 addr, const u64 data);
	void Write128(const u64 addr, const u128 data);

	u8 Read8(const u64 addr);
	u16 Read16(const u64 addr);
	u32 Read32(const u64 addr);
	u64 Read64(const u64 addr);
	u128 Read128(const u64 addr);

	template<typename T> void WriteData(const u64 addr, const T* data)
	{
		memcpy(GetMemFromAddr(addr), data, sizeof(T));
	}

	template<typename T> void WriteData(const u64 addr, const T data)
	{

		*(T*)GetMemFromAddr(addr) = data;
	}

	wxString ReadString(const u64 addr, const u64 len)
	{
		wxString str;
		str.Clear();
		wxStringBuffer buf(str, len);
		memcpy((wxChar*)buf, (const void*)GetMemFromAddr(addr), len);

		return str;
	}

	wxString ReadString(const u64 addr)
	{
		wxString buf = wxEmptyString;

		for(u32 i=addr; ; i++)
		{
			const u8 c = Read8(i);
			if(c == 0) break;
			buf += c;
		}

		return buf;
	}

	void WriteString(const u64 addr, const wxString& str)
	{
		for(u32 i=0; i<str.Length(); i++)
		{
			Write8(addr + i, str[i]);
		}

		Write8(addr + str.Length(), 0);
	}

	static u64 AlignAddr(const u64 addr, const u8 align)
	{
		return (addr + (align-1)) & ~(align-1);
	}

	u32 GetUserMemTotalSize()
	{
		return UserMem.GetSize();
	}

	u32 GetUserMemAvailSize()
	{
		u32 used_usermem_size = 0;
		for(u32 i=0; i<m_used_usermem.GetCount(); ++i) used_usermem_size += m_used_usermem[i].size;
		return UserMem.GetSize() - used_usermem_size;
	}

	void CombineFreeMem()
	{
		if(m_free_usermem.GetCount() < 2) return;

		for(u32 i1=0; i1<m_free_usermem.GetCount(); ++i1)
		{
			UserMemInfo& u1 = m_free_usermem[i1];
			for(u32 i2=i1+1; i2<m_free_usermem.GetCount(); ++i2)
			{
				const UserMemInfo u2 = m_free_usermem[i2];
				if(u1.addr + u1.size != u2.addr) continue;
				u1.size += u2.size;
				m_free_usermem.RemoveAt(i2);
				break;
			}
		}
	}

	u64 Alloc(const u32 size, const u32 align)
	{
		if(GetUserMemAvailSize() < size)
		{
			ConLog.Error("Not enought free user mem");
			return 0;
		}

		for(u32 i=0; i<m_free_usermem.GetCount(); ++i)
		{
			if(m_free_usermem[i].size < size) continue;
			UserMemInfo mem(m_free_usermem[i].addr, size);
			m_used_usermem.AddCpy(mem);

			if(m_free_usermem[i].size == size)
			{
				m_free_usermem.RemoveAt(i);
			}
			else
			{
				m_free_usermem[i].addr += size;
				m_free_usermem[i].size -= size;
			}

			memset(GetMemFromAddr(mem.addr), 0, mem.size);
			return mem.addr;
		}

		UserMemInfo mem(m_mem_point, size);
		ConLog.Warning("Memory alloc: creating new block (addr=0x%llx,size=0x%x)", mem.addr, mem.size);
		if(!IsGoodAddr(mem.addr, mem.size)) return 0;
		memset(GetMemFromAddr(mem.addr), 0, mem.size);
		m_used_usermem.AddCpy(mem);
		m_mem_point += size;
		return mem.addr;
	}

	bool Free(const u64 addr)
	{
		for(u32 i=0; i<m_used_usermem.GetCount(); ++i)
		{
			if(m_used_usermem[i].addr != addr) continue;
			m_free_usermem.AddCpy(m_used_usermem[i]);
			m_used_usermem.RemoveAt(i);
			CombineFreeMem();
			return true;
		}

		return false;
	}
};

extern MemoryBase Memory;