#pragma once

typedef int ContentLocT;

struct SongMgr
{
	SongMgr();
	~SongMgr();

	int AddSongData(DataArray*, const char*, ContentLocT);
	int Handle(DataArray*, bool);

	const char* Bank(Symbol);
	int BasePoints(Symbol);
	DataArray* Data(Symbol);
	const char* Decade(Symbol);
	const char* Genre(Symbol);
	
	int IsDownload(Symbol, DataArray*);
	int IsExported(Symbol, DataArray*);
	int IsFake(Symbol, DataArray*);
	int IsPrivate(Symbol, DataArray*);
	int IsOnDisc(Symbol, DataArray*);
	
	int IsMaster(Symbol);
	// int Rank(Symbol*, Symbol*, SongType);
	int RankTier(Symbol, Symbol);
	int Song(int);

	const char* SongAlbum(Symbol);
	int SongAlbumTrack(Symbol);
	const char* SongArtist(Symbol*);
	const char* SongName(Symbol*);
	const char* SongPack(Symbol);
	int TuningOffset(Symbol);
	const char* VocalGender(Symbol);
	int YearReleased(Symbol);
};

struct WiiSongMgr : SongMgr
{
	int Handle(DataArray*, bool);
	int Load(BinStream*);
};

struct SongOffer
{
	u8 unknown[0x28];
	DataArray* data;
};

struct SongOfferProvider
{
	const char* DataSymbol(int);
	SongOffer* GetSongOffer(Symbol);
};

extern SongMgr gSongMgr;
extern WiiSongMgr gSongMgrWii;

