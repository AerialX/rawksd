using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Harmonix;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	public static class MetadataFormatHarmonix
	{
		public static SongsDTA GetSongsDTA(SongData song)
		{
			return SongsDTA.Create(song.Data.GetSubtree("HmxSongsDtb").GetSubtree());
		}

		public static SongsDTA GetSongData(SongData data)
		{
			SongsDTA dta = new SongsDTA();
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
			dta.Song.HopoThreshold = data.HopoThreshold;
			for (Instrument instrument = Instrument.Ambient; instrument <= Instrument.Vocals; instrument++) {
				SongsDTA.Rankings rank = dta.Rank.SingleOrDefault(r => r.Name == InstrumentToString(instrument));
				if (rank != null)
					rank.Rank = data.Difficulty[instrument];
			}

			return dta;
		}

		public static SongData GetSongData(DTB.NodeTree dtb)
		{
			SongData data = new SongData();

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
			if (dta.Song.HopoThreshold.HasValue)
				data.HopoThreshold = dta.Song.HopoThreshold.Value;

			return data;
		}

		public static AudioFormat GetAudioFormat(FormatData data)
		{
			AudioFormat format = new AudioFormat();

			SongsDTA dta = GetSongData(data.Song);

			for (int i = 0; i < dta.Song.Vols.Count; i++) {
				AudioFormat.Mapping map = new AudioFormat.Mapping();
				map.Balance = dta.Song.Pans[i];
				map.Volume = dta.Song.Vols[i];
				SongsDTA.SongTracks track = dta.Song.Tracks.SingleOrDefault(s => s.Tracks.Contains(i));
				if (track != null)
					map.Instrument = Platform.InstrumentFromString(track.Name);
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
	}
}
