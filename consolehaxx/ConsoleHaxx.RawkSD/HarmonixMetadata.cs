using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Harmonix;
using System.IO;
using ConsoleHaxx.Common;
using System.Text.RegularExpressions;

namespace ConsoleHaxx.RawkSD
{
	public static class HarmonixMetadata
	{
		public static SongsDTA GetSongsDTA(SongData song)
		{
			DataArray tree = song.Data.GetSubtree("HmxSongsDtb");
			if (tree != null)
				return SongsDTA.Create(tree.GetSubtree());
			return null;
		}

		public static void SetSongsDTA(SongData song, DTB.NodeTree dtb)
		{
			song.Data.SetSubtree("HmxSongsDtb", dtb);
		}

		public static SongsDTA GetSongData(SongData data)
		{
			SongsDTA dta = GetSongsDTA(data) ?? new SongsDTA();
			dta.Album = data.Album;
			dta.Track = data.AlbumTrack;
			dta.Artist = data.Artist;
			dta.BaseName = data.ID;
			dta.Genre = data.Genre;
			dta.Master = data.Master;
			dta.Name = data.Name;
			dta.Pack = data.Pack;
			dta.Vocalist = data.Vocalist;
			dta.Year = data.Year;
			dta.Version = data.Version;
			if (data.HopoThreshold > 0)
				dta.Song.HopoThreshold = data.HopoThreshold;
			for (Instrument instrument = Instrument.Ambient; instrument <= Instrument.Vocals; instrument++) {
				SongsDTA.Rankings rank = dta.Rank.SingleOrDefault(r => r.Name == InstrumentToString(instrument));
				if (rank != null)
					rank.Rank = data.Difficulty[instrument];
			}

			return dta;
		}

		public static SongData GetSongData(PlatformData platformdata, DTB.NodeTree dtb)
		{
			SongData data = new SongData(platformdata);

			data.Data.SetSubtree("HmxSongsDtb", dtb);

			SongsDTA dta = SongsDTA.Create(dtb);
			data.Album = dta.Album;
			if (dta.Track.HasValue)
				data.AlbumTrack = dta.Track.Value;
			data.Artist = dta.Artist;
			data.ID = dta.BaseName;
			for (Instrument instrument = Instrument.Ambient; instrument <= Instrument.Vocals; instrument++) {
				SongsDTA.Rankings rank = dta.Rank.SingleOrDefault(r => r.Name == InstrumentToString(instrument));
				if (rank != null)
					data.Difficulty[instrument] = rank.Rank;
			}
			data.Genre = dta.Genre;
			data.Master = dta.Master;
			data.Name = dta.Name;
			data.Pack = dta.Pack;
			data.Vocalist = dta.Vocalist;
			data.Year = dta.Year;
			data.Version = dta.Version;
			data.PreviewTimes = dta.Preview;
			if (dta.Song.HopoThreshold.HasValue)
				data.HopoThreshold = dta.Song.HopoThreshold.Value;

			return data;
		}

		public static AudioFormat GetAudioFormat(SongData song)
		{
			AudioFormat format = new AudioFormat();

			SongsDTA dta = GetSongsDTA(song);
			if (dta == null)
				return null;

			bool notracks = dta.Song.Tracks.Sum(t => t.Tracks.Count) == 0;

			for (int i = 0; i < dta.Song.Vols.Count; i++) {
				AudioFormat.Mapping map = new AudioFormat.Mapping();
				map.Balance = dta.Song.Pans[i];
				map.Volume = dta.Song.Vols[i];
				SongsDTA.SongTracks track = dta.Song.Tracks.SingleOrDefault(s => s.Tracks.Contains(i));
				if (track != null)
					map.Instrument = Platform.InstrumentFromString(track.Name);
				else if (notracks && dta.Song.Cores[i] == 1)
					map.Instrument = Instrument.Guitar;
				format.Mappings.Add(map);
			}

			return format;
		}

		public static void SaveAudioFormat(AudioFormat data, FormatData destination, int id)
		{
			SongsDTA dta = new SongsDTA();

			for (int i = 0; i < data.Channels; i++) {
				AudioFormat.Mapping map = data.Mappings[i];
				dta.Song.Vols.Add(map.Volume);
				dta.Song.Pans.Add(map.Balance);
				dta.Song.Cores.Add(map.Instrument == Instrument.Guitar ? 1 : -1);
				string trackname = InstrumentToString(map.Instrument);
				SongsDTA.SongTracks track = dta.Song.Tracks.SingleOrDefault(t => t.Name == trackname);
				if (track != null)
					track.Tracks.Add(i);
			}

			destination.Song.Data.SetSubtree("HmxSongsDtb", dta.ToDTB());
		}

		public static Ark GetHarmonixArk(DirectoryNode dir)
		{
			DirectoryNode gen = dir.Navigate("gen", false, true) as DirectoryNode;
			if (gen == null) // Just in case we're given the "wrong" directory that directly contains the ark
				gen = dir;

			List<Pair<int, Stream>> arkfiles = new List<Pair<int, Stream>>();
			Stream hdrfile = null;
			foreach (FileNode file in gen.Children.Where(n => n is FileNode)) {
				if (file.Name.ToLower().EndsWith(".hdr"))
					hdrfile = file.Data;
				else if (file.Name.ToLower().EndsWith(".ark")) {
					Match match = Regex.Match(file.Name.ToLower(), @"_(\d+).ark");
					if (match.Success)
						arkfiles.Add(new Pair<int, Stream>(int.Parse(match.Groups[1].Value), file.Data));
					else
						arkfiles.Add(new Pair<int, Stream>(0, file.Data));
				}
			}

			// FreQuency/Amplitude where the header is the ark
			if (hdrfile == null) {
				if (arkfiles.Count == 1) {
					hdrfile = arkfiles[0].Value;
					arkfiles.Clear();
				} else
					throw new FormatException();
			}

			return new Ark(new EndianReader(hdrfile, Endianness.LittleEndian), arkfiles.OrderBy(f => f.Key).Select(f => f.Value).ToArray());
		}

		public static string InstrumentToString(Instrument instrument)
		{
			switch (instrument) {
				case Instrument.Guitar:
					return "guitar";
				case Instrument.Bass:
					return "bass";
				case Instrument.Drums:
					return "drum";
				case Instrument.Vocals:
					return "vocals";
				case Instrument.Ambient:
					return "band";
			}

			return null;
		}

		public static bool IsRockBand1(Game game)
		{
			switch (game) {
				case Game.RockBand:
				case Game.RockBandACDC:
				case Game.RockBandTP1:
				case Game.RockBandTP2:
				case Game.RockBandMetalTP:
				case Game.RockBandCountryTP:
				case Game.RockBandClassicTP:
					return true;
			}

			return false;
		}

		public static bool IsGuitarHero12(Game game)
		{
			switch (game) {
				case Game.GuitarHero1:
				case Game.GuitarHero2:
				case Game.GuitarHero80s:
					return true;
			}

			return false;
		}
	}
}
