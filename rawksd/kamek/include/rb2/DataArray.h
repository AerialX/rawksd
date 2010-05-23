#pragma once

typedef int DataType; // DTB type

struct DataArray;

struct DataNode
{
	DataArray* data;	// 0x00
	DataType type;		// 0x04

	DataNode(DataArray*, DataType);
	DataNode(DataNode*);
	DataNode(String*);
	DataNode(const char*);
	DataNode(int);
	DataNode(const void*, int);

	~DataNode();

	int Construct(DataArray*, DataType);
	int Construct(DataNode*);
	int Construct(String*);
	int Construct(const char*);
	int Construct(int);
	int Construct(const void*, int);
	int Construct();

	int Destruct();

	int Command(DataArray*);
	DataArray* Array(DataArray*);
	float Float(DataArray*);
	int Int(DataArray*);
	const char* Str(DataArray*);
	Symbol* Sym(DataArray*);

	DataArray* LiteralArray(DataArray*);
	float LiteralFloat(DataArray*);
	int LiteralInt(DataArray*);
	const char* LiteralStr(DataArray*);
	Symbol* LiteralSym(DataArray*);
	
	static DataNode* Alloc()
	{
		return (DataNode*)PoolAlloc(0x10, 0x10);
	}

	void Free()
	{
		Destruct();
		PoolFree(0x10, this);
	}
};

struct DataArray
{
	DataNode* nodes;	// 0x00
	const char* name;	// 0x04
	u16 size;			// 0x08
	s16 refcount;		// 0x0A
	u16 line;			// 0x0C
	u16 unknown2;		// 0x0E

	static DataArray* Alloc()
	{
		return (DataArray*)PoolAlloc(0x10, 0x10);
	}

	DataArray(const void*, int);
	DataArray(int);
	~DataArray();
	void TryDestruct()
	{
		refcount--;
		if (refcount <= 0)
			this->~DataArray();
	}

	int Load(BinStream*);
	int Save(BinStream*);
	
	int Insert(int, DataNode&);
	int InsertNodes(int, DataArray*);

	int Int(int);

	int Remove(DataNode&);
	int Remove(int);
	int Resize(int);

	int Size();
	int SortNodes();

	DataArray* Clone(bool, bool);

	DataArray* FindArray(Symbol, Symbol);
	DataArray* FindArray(Symbol, Symbol, Symbol);
	DataArray* FindArray(Symbol, bool);
	DataArray* FindArray(Symbol, const char*);
	DataArray* FindArray(int, bool);
	int FindData(Symbol, bool*, bool);
	int FindData(Symbol, const char*&, bool);
	int FindData(Symbol, float*, bool);
	int FindData(Symbol, int*, bool);
};

int __rs(BinStream*, DataArray*&);
int __ls(TextStream*, const DataArray*);

DataArray* SystemConfig(Symbol, Symbol);

