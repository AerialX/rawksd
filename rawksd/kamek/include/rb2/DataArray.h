#pragma once

typedef int DataType; // DTB type

struct DataArray;

struct DataNode
{
	u8 data[0x10];

	int Command(DataArray*);

	int Construct(DataArray*, DataType);
	int Construct(DataNode*);
	int Construct(String*);
	int Construct(const char*);
	int Construct(int);
	int Construct(const void*, int);
	int Construct();

	int Destruct();

	DataArray* Array(DataArray*);
	
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
	DataNode* node;
	u8 data[0x0C];

	static DataArray* Alloc()
	{
		return (DataArray*)PoolAlloc(0x10, 0x10);
	}

	int Construct(const void*, int);
	int Construct(int);
	int Destruct();

	int Load(BinStream*);
	int Save(BinStream*);
	
	int Insert(int, DataNode*);
	int InsertNodes(int, DataArray*);

	int Int(int);

	int Remove(DataNode*);
	int Remove(int);
	int Resize(int);

	int Size();
	int SortNodes();

	DataArray* Clone(bool, bool);

	int FindArray(Symbol*, Symbol*);
	int FindArray(Symbol*, Symbol*, Symbol*);
	int FindArray(Symbol*, bool);
	int FindArray(Symbol, const char*);
	int FindArray(int, bool);
	int FindData(Symbol*, bool*, bool);
	int FindData(Symbol*, const char**, bool);
	int FindData(Symbol, float*, bool);
	int FindData(Symbol, int*, bool);
};

int __rs(BinStream*, DataArray**);
int __ls(TextStream*, const DataArray*);

DataArray* SystemConfig(Symbol*, Symbol*);

