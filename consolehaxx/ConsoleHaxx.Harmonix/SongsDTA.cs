using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;
using ConsoleHaxx.Common;
using System.Globalization;

namespace ConsoleHaxx.Harmonix
{
	public class SongsDTA
	{
		public SongsDTA()
		{
			Context = 105;
			Version = 1;
			Format = 3;
			AnimTempo = 0x20;
			Year = 2009;
			BasePoints = 50000;
			SongScrollSpeed = 2300;
			Bank = "sfx/tambourine_bank.milo";
			Genre = "rock";
			Album = "";
			Artist = "";
			Name = "Custom";
			Vocalist = "male";
			Master = true;
		}

		public string BaseName = string.Empty;
		public string Name = string.Empty;
		public string Artist = string.Empty;
		public bool Master;
		public int Context;
		public SongInfo Song = new SongInfo();
		public SongInfo SongCoop;
		public int? TuningOffset;
		public float? TuningOffsetCents;
		public int SongScrollSpeed;
		public string Bank = string.Empty;
		public int AnimTempo;
		public List<int> Preview = new List<int>() { 0, 0 };
		public List<Rankings> Rank = new List<Rankings>() { new Rankings() { Name = "drum" }, new Rankings() { Name = "guitar" }, new Rankings() { Name = "bass" }, new Rankings() { Name = "vocals" }, new Rankings() { Name = "band" } };
		public string Genre = string.Empty;
		public string Decade = string.Empty;
		public string Vocalist = string.Empty;
		public List<string> VideoVenues = new List<string>();
		public int Version;
		public int Format;
		public bool Downloaded;
		public bool? Exported;
		public bool? AlbumArt;
		public int Year;
		public string Album = string.Empty;
		public int? Track;
		public string Pack;
		public int BasePoints;

		// RBN stuff
		public int? Rating;
		public int? SongID;
		public string Origin;
		public string SubGenre;
		public int? Ugc;
		public int? SongLength;

		public class Rankings
		{
			public string Name = string.Empty;
			public int Rank;
		}

		public class SongInfo
		{
			public SongInfo()
			{

			}

			public string Name = string.Empty;
			public List<SongTracks> Tracks = new List<SongTracks>() { new SongTracks() { Name = "drum" }, new SongTracks() { Name = "bass" }, new SongTracks() { Name = "guitar" }, new SongTracks() { Name = "vocals" } };
			public List<float> Pans = new List<float>() { -1, 1 };
			public List<float> Vols = new List<float>() { -1, -1 };
			public List<int> Cores = new List<int>() { -1, -1 };
			public List<string> DrumSolo = new List<string>() { "kick.cue", "snare.cue", "tom1.cue", "tom2.cue", "crash.cue" };
			public List<string> DrumFreestyle = new List<string>() { "kick.cue", "snare.cue", "hat.cue", "ride.cue", "crash.cue" };
			public string MidiFile = string.Empty;
			public int? HopoThreshold;

			// RBN stuff
			public List<int> TracksCount = new List<int>();
		}

		public class SongTracks
		{
			public SongTracks()
			{

			}

			public string Name = string.Empty;
			public List<int> Tracks = new List<int>();
		}

		public DTB.NodeTree ToDTB(bool rawksd2 = false)
		{
			uint line = 1;
			return ToDTB(ref line, rawksd2);
		}

		public DTB.NodeTree ToDTB(ref uint line, bool rawksd2 = false)
		{
			DTB.NodeTree tree = new DTB.NodeTree(line++);
			tree.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = BaseName });
			DTB.NodeTree name = new DTB.NodeTree(line++); tree.Nodes.Add(name);
			name.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "name" });
			name.Nodes.Add(new DTB.NodeString() { Type = 0x12, Text = Name });
			DTB.NodeTree artist = new DTB.NodeTree(line++); tree.Nodes.Add(artist);
			artist.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "artist" });
			artist.Nodes.Add(new DTB.NodeString() { Type = 0x12, Text = Artist });
			DTB.NodeTree master = new DTB.NodeTree(line++); tree.Nodes.Add(master);
			master.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "master" });
			master.Nodes.Add(new DTB.NodeInt32() { Number = Master ? 1 : 0 });
			DTB.NodeTree song = new DTB.NodeTree(line++); tree.Nodes.Add(song);
			song.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "song" });
			name = new DTB.NodeTree(line++); song.Nodes.Add(name);
			name.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "name" });
			name.Nodes.Add(new DTB.NodeString() { Type = 0x05, Text = Song.Name });
			if (Song.TracksCount.Count > 0) {
				DTB.NodeTree trackscount = new DTB.NodeTree(line++); song.Nodes.Add(trackscount);
				trackscount.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "tracks_count" });
				DTB.NodeTree trackstree = new DTB.NodeTree(line++); trackscount.Nodes.Add(trackstree);
				foreach (int trackcount in Song.TracksCount)
					trackstree.Nodes.Add(new DTB.NodeInt32() { Number = trackcount });
			}
			DTB.NodeTree tracks = new DTB.NodeTree(line++); song.Nodes.Add(tracks);
			tracks.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "tracks" });
			DTB.NodeTree tracktree = new DTB.NodeTree(line); tracks.Nodes.Add(tracktree);
			foreach (SongTracks track in Song.Tracks) {
				if (rawksd2) {
					int drumtrackcount = 6; // For safety with customs messing with mix and not knowing what they're doing
					if (track.Name == "drum" && track.Tracks.Count < drumtrackcount && track.Tracks.Count > 0) {
						for (int k = 0; k < drumtrackcount - track.Tracks.Count; k++)
							track.Tracks.Add(track.Tracks[track.Tracks.Count - 1]);
						// Now there are duplicates... I blame RB2.
					}
				} else if (track.Tracks.Count == 0)
					continue;
				tracks = new DTB.NodeTree(line++); tracktree.Nodes.Add(tracks);
				tracks.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = track.Name });
				if (track.Tracks.Count == 1)
					tracks.Nodes.Add(new DTB.NodeInt32() { Number = track.Tracks[0] });
				else {
					DTB.NodeTree trk = new DTB.NodeTree(line - 1); tracks.Nodes.Add(trk);
					foreach (int num in track.Tracks)
						trk.Nodes.Add(new DTB.NodeInt32() { Number = num });
				}
			}
			line += 3;
			DTB.NodeTree pans = new DTB.NodeTree(line); song.Nodes.Add(pans);
			pans.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "pans" });
			DTB.NodeTree values = new DTB.NodeTree(line++); pans.Nodes.Add(values);
			foreach (float pan in Song.Pans)
				values.Nodes.Add(new DTB.NodeFloat32() { Number = pan });
			DTB.NodeTree vols = new DTB.NodeTree(line); song.Nodes.Add(vols);
			vols.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "vols" });
			values = new DTB.NodeTree(line++); vols.Nodes.Add(values);
			foreach (float vol in Song.Vols)
				values.Nodes.Add(new DTB.NodeFloat32() { Number = vol });
			DTB.NodeTree cores = new DTB.NodeTree(line); song.Nodes.Add(cores);
			cores.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "cores" });
			values = new DTB.NodeTree(line++); cores.Nodes.Add(values);
			foreach (int core in Song.Cores)
				values.Nodes.Add(new DTB.NodeInt32() { Number = core });
			DTB.NodeTree drumsolo = new DTB.NodeTree(line++); song.Nodes.Add(drumsolo);
			drumsolo.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "drum_solo" });
			DTB.NodeTree seqs = new DTB.NodeTree(line); drumsolo.Nodes.Add(seqs);
			seqs.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "seqs" });
			values = new DTB.NodeTree(line++); seqs.Nodes.Add(values);
			foreach (string seq in Song.DrumSolo)
				values.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = seq });
			line++;
			DTB.NodeTree drumfreestyle = new DTB.NodeTree(line++); song.Nodes.Add(drumfreestyle);
			drumfreestyle.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "drum_freestyle" });
			seqs = new DTB.NodeTree(line); drumfreestyle.Nodes.Add(seqs);
			seqs.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "seqs" });
			values = new DTB.NodeTree(line++); seqs.Nodes.Add(values);
			foreach (string seq in Song.DrumFreestyle)
				values.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = seq });
			line++;
			DTB.NodeTree midi = new DTB.NodeTree(line++); song.Nodes.Add(midi);
			midi.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "midi_file" });
			midi.Nodes.Add(new DTB.NodeString() { Type = 0x12, Text = Song.MidiFile });
			if (Song.HopoThreshold.HasValue) {
				DTB.NodeTree hopothreshold = new DTB.NodeTree(line++); song.Nodes.Add(hopothreshold);
				hopothreshold.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "hopo_threshold" });
				hopothreshold.Nodes.Add(new DTB.NodeInt32() { Number = (int)Song.HopoThreshold });
			}
			line++;
			DTB.NodeTree songscrollspeed = new DTB.NodeTree(line++); tree.Nodes.Add(songscrollspeed);
			songscrollspeed.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "song_scroll_speed" });
			songscrollspeed.Nodes.Add(new DTB.NodeInt32() { Number = SongScrollSpeed });
			DTB.NodeTree bank = new DTB.NodeTree(line++); tree.Nodes.Add(bank);
			bank.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "bank" });
			bank.Nodes.Add(new DTB.NodeString() { Type = 0x12, Text = Bank });
			DTB.NodeTree animtempo = new DTB.NodeTree(line++); tree.Nodes.Add(animtempo);
			animtempo.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "anim_tempo" });
			animtempo.Nodes.Add(new DTB.NodeInt32() { Number = AnimTempo }); // TODO: Animation speed...
			if (SongLength.HasValue) {
				DTB.NodeTree songlength = new DTB.NodeTree(line++); tree.Nodes.Add(songlength);
				songlength.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "song_length" });
				songlength.Nodes.Add(new DTB.NodeInt32() { Number = SongLength.Value });
			}
			DTB.NodeTree preview = new DTB.NodeTree(line++); tree.Nodes.Add(preview);
			preview.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "preview" });
			foreach (int prev in Preview)
				preview.Nodes.Add(new DTB.NodeInt32() { Number = prev });
			DTB.NodeTree ranks = new DTB.NodeTree(line++); tree.Nodes.Add(ranks);
			ranks.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "rank" });
			foreach (Rankings r in Rank) {
				DTB.NodeTree rnk = new DTB.NodeTree(line++); ranks.Nodes.Add(rnk);
				rnk.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = r.Name });
				rnk.Nodes.Add(new DTB.NodeInt32() { Number = r.Rank });
			}
			line++;
			DTB.NodeTree genre = new DTB.NodeTree(line++); tree.Nodes.Add(genre);
			genre.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "genre" });
			genre.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = Genre });
			DTB.NodeTree decade = new DTB.NodeTree(line++); tree.Nodes.Add(decade);
			decade.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "decade" });
			decade.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = Decade });
			DTB.NodeTree vocal_gender = new DTB.NodeTree(line++); tree.Nodes.Add(vocal_gender);
			vocal_gender.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "vocal_gender" });
			vocal_gender.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = Vocalist });
			if (VideoVenues.Count > 0) {
				DTB.NodeTree video_venues = new DTB.NodeTree(line); tree.Nodes.Add(video_venues);
				video_venues.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "video_venues" });
				DTB.NodeTree venuestree = new DTB.NodeTree(line++); video_venues.Nodes.Add(venuestree);
				foreach (string s in VideoVenues)
					venuestree.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = s });
			}
			line++;
			DTB.NodeTree version = new DTB.NodeTree(line++); tree.Nodes.Add(version);
			version.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "version" });
			version.Nodes.Add(new DTB.NodeInt32() { Number = Version });
			DTB.NodeTree downloaded = new DTB.NodeTree(line++); tree.Nodes.Add(downloaded);
			downloaded.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "downloaded" });
			downloaded.Nodes.Add(new DTB.NodeInt32() { Number = Downloaded ? 1 : 0 });
			line++;
			DTB.NodeTree format = new DTB.NodeTree(line++); tree.Nodes.Add(format);
			format.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "format" });
			format.Nodes.Add(new DTB.NodeInt32() { Number = Format });
			line++;
			if (Exported.HasValue) {
				DTB.NodeTree exported = new DTB.NodeTree(line++); tree.Nodes.Add(downloaded);
				exported.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "exported" });
				exported.Nodes.Add(new DTB.NodeInt32() { Number = Exported.Value ? 1 : 0 });
			}
			if (AlbumArt.HasValue) {
				DTB.NodeTree album_art = new DTB.NodeTree(line++); tree.Nodes.Add(album_art);
				album_art.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "album_art" });
				album_art.Nodes.Add(new DTB.NodeInt32() { Number = AlbumArt.Value ? 1 : 0 });
			}
			DTB.NodeTree year = new DTB.NodeTree(line++); tree.Nodes.Add(year);
			year.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "year_released" });
			year.Nodes.Add(new DTB.NodeInt32() { Number = Year });
			if (Album != null) {
				DTB.NodeTree album = new DTB.NodeTree(line++); tree.Nodes.Add(album);
				album.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "album_name" });
				album.Nodes.Add(new DTB.NodeString() { Type = 0x12, Text = Album });
			}
			//if (!Track.HasValue) Track = 1;
			if (Track.HasValue) {
				DTB.NodeTree tracknum = new DTB.NodeTree(line++); tree.Nodes.Add(tracknum);
				tracknum.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "album_track_number" });
				tracknum.Nodes.Add(new DTB.NodeInt32() { Number = Track.Value });
			}
			if (Pack != null) {
				DTB.NodeTree packname = new DTB.NodeTree(line++); tree.Nodes.Add(packname);
				packname.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "pack_name" });
				packname.Nodes.Add(new DTB.NodeString() { Type = 0x12, Text = Pack });
			} else
				line++;
			DTB.NodeTree basepoints = new DTB.NodeTree(line++); tree.Nodes.Add(basepoints);
			basepoints.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "base_points" });
			basepoints.Nodes.Add(new DTB.NodeInt32() { Number = BasePoints });
			// RBN
			if (Rating.HasValue) {
				DTB.NodeTree rating = new DTB.NodeTree(line++); tree.Nodes.Add(rating);
				rating.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "rating" });
				rating.Nodes.Add(new DTB.NodeInt32() { Number = Rating.Value });
			}
			if (SubGenre != null) {
				DTB.NodeTree subgenre = new DTB.NodeTree(line++); tree.Nodes.Add(subgenre);
				subgenre.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "sub_genre" });
				subgenre.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = SubGenre });
			}
			if (SongID.HasValue) {
				DTB.NodeTree songid = new DTB.NodeTree(line++); tree.Nodes.Add(songid);
				songid.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "song_id" });
				songid.Nodes.Add(new DTB.NodeInt32() { Number = SongID.Value });
			}
			if (TuningOffset.HasValue) {
				DTB.NodeTree tuningoffset = new DTB.NodeTree(line++); tree.Nodes.Add(tuningoffset);
				tuningoffset.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "tuning_offset" });
				tuningoffset.Nodes.Add(new DTB.NodeInt32() { Number = (int)TuningOffset });
			}
			if (TuningOffsetCents.HasValue) {
				DTB.NodeTree tuningoffsetcents = new DTB.NodeTree(line++); tree.Nodes.Add(tuningoffsetcents);
				tuningoffsetcents.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "tuning_offset_cents" });
				tuningoffsetcents.Nodes.Add(new DTB.NodeFloat32() { Number = (float)TuningOffsetCents });
			}
			DTB.NodeTree context = new DTB.NodeTree(line++); tree.Nodes.Add(context);
			context.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "context" });
			context.Nodes.Add(new DTB.NodeInt32() { Number = Context });
			if (Origin != null) {
				DTB.NodeTree origin = new DTB.NodeTree(line++); tree.Nodes.Add(origin);
				origin.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "game_origin" });
				origin.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = Origin });
			}
			if (Ugc.HasValue) {
				DTB.NodeTree ugc = new DTB.NodeTree(line++); tree.Nodes.Add(ugc);
				ugc.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = "ugc" });
				ugc.Nodes.Add(new DTB.NodeInt32() { Number = Ugc.Value });
			}

			line += 2;
			return tree;
		}

		public static SongsDTA Create(DTB.NodeTree tree)
		{
			SongsDTA dta = new SongsDTA();
			dta.BaseName = (tree.Nodes[0] as DTB.NodeString).Text;

			dta.Preview.Clear();
			//dta.Song.Tracks.Clear();
			dta.Song.DrumSolo.Clear();
			dta.Song.DrumFreestyle.Clear();
			dta.Song.Pans.Clear();
			dta.Song.Vols.Clear();
			dta.Song.Cores.Clear();
			dta.Song.TracksCount.Clear();

			List<DTB.Node> subtrees = tree.Nodes.FindAll(n => n is DTB.NodeTree);
			foreach (DTB.NodeTree subtree in subtrees) {
				switch ((subtree.Nodes[0] as DTB.NodeString).Text) {
					case "name":
						dta.Name = (subtree.Nodes[1] as DTB.NodeString).Text;
						break;
					case "artist":
						dta.Artist = (subtree.Nodes[1] as DTB.NodeString).Text;
						break;
					case "master":
						if (subtree.Nodes[1] is DTB.NodeString)
							dta.Master = (subtree.Nodes[1] as DTB.NodeString).Text == "TRUE";
						else
							dta.Master = (subtree.Nodes[1] as DTB.NodeInt32).Number == 1;
						break;
					case "context":
						dta.Context = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "song":
					case "song_coop":
						SongsDTA.SongInfo song = (subtree.Nodes[0] as DTB.NodeString).Text == "song" ? dta.Song : (dta.SongCoop = new SongInfo());
						foreach (DTB.NodeTree n in subtree.Nodes.FindAll(n => n is DTB.NodeTree)) {
							switch ((n.Nodes[0] as DTB.NodeString).Text) {
								case "name":
									song.Name = (n.Nodes[1] as DTB.NodeString).Text;
									break;
								case "tracks_count":
									foreach (DTB.NodeInt32 i in (n.Nodes[1] as DTB.NodeTree).Nodes)
										song.TracksCount.Add(i.Number);
									break;
								case "tracks":
									if (n.Nodes[1] is DTB.NodeTree) {
										foreach (DTB.NodeTree t in (n.Nodes[1] as DTB.NodeTree).Nodes) {
											string name = (t.Nodes[0] as DTB.NodeString).Text;
											if (name == "rhythm") // GH2 hack
												name = "bass";
											if (t.Nodes[1] is DTB.NodeInt32)
												song.Tracks.Find(t2 => t2.Name == name).Tracks = new List<int>() { (t.Nodes[1] as DTB.NodeInt32).Number };
											//dta.Song.Tracks.Add(new SongsDTA.SongTracks() { Name = name, Tracks = new List<int>() { (t.Nodes[1] as DTB.NodeInt32).Number } });
											else {
												SongsDTA.SongTracks songtrack = song.Tracks.Find(t2 => t2.Name == name);
												//SongsDTA.SongTracks songtrack = new SongsDTA.SongTracks() { Name = name, Tracks = new List<int>() };
												(t.Nodes[1] as DTB.NodeTree).Nodes.ForEach(l => {
													songtrack.Tracks.Add((l as DTB.NodeInt32).Number);
												});
												//dta.Song.Tracks.Add(songtrack);
											}
										}
									}
									break;
								case "pans":
									foreach (DTB.Node nd in (n.Nodes[1] as DTB.NodeTree).Nodes)
										if (nd is DTB.NodeFloat32) {
											song.Pans.Add((nd as DTB.NodeFloat32).Number);
										} else {
											song.Pans.Add((nd as DTB.NodeInt32).Number);
										}
									break;
								case "vols":
									foreach (DTB.NodeFloat32 f in (n.Nodes[1] as DTB.NodeTree).Nodes)
										song.Vols.Add(f.Number);
									break;
								case "cores":
									foreach (DTB.NodeInt32 i in (n.Nodes[1] as DTB.NodeTree).Nodes)
										song.Cores.Add(i.Number);
									break;
								case "drum_solo":
									foreach (DTB.NodeString s in ((n.Nodes[1] as DTB.NodeTree).Nodes[1] as DTB.NodeTree).Nodes)
										song.DrumSolo.Add(s.Text);
									break;
								case "drum_freestyle":
									foreach (DTB.NodeString s in ((n.Nodes[1] as DTB.NodeTree).Nodes[1] as DTB.NodeTree).Nodes)
										song.DrumFreestyle.Add(s.Text);
									break;
								case "midi_file":
									song.MidiFile = ((n as DTB.NodeTree).Nodes[1] as DTB.NodeString).Text;
									break;
								case "hopo_threshold":
									song.HopoThreshold = ((n as DTB.NodeTree).Nodes[1] as DTB.NodeInt32).Number;
									break;
							}
						};
						break;
					case "tuning_offset":
						dta.TuningOffset = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "tuning_offset_cents":
						if (subtree.Nodes[1] is DTB.NodeInt32)
							dta.TuningOffsetCents = (float)(subtree.Nodes[1] as DTB.NodeInt32).Number;
						else
							dta.TuningOffsetCents = (subtree.Nodes[1] as DTB.NodeFloat32).Number;
						break;
					case "song_scroll_speed":
						dta.SongScrollSpeed = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "bank":
						dta.Bank = (subtree.Nodes[1] as DTB.NodeString).Text;
						break;
					case "anim_tempo":
						if (subtree.Nodes[1] is DTB.NodeString)
							//dta.AnimTempo = (subtree.Nodes[1] as DTB.NodeString).Text;
							dta.AnimTempo = 0x20;
						else
							dta.AnimTempo = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "preview":
						dta.Preview.Add((subtree.Nodes[1] as DTB.NodeInt32).Number);
						dta.Preview.Add((subtree.Nodes[2] as DTB.NodeInt32).Number);
						break;
					case "preview_clip":
						//dta.PreviewClip = (subtree.Nodes[1] as DTB.NodeString).Text;
						break;
					case "rank":
						subtree.Nodes.FindAll(n => n is DTB.NodeTree).ForEach(n => {
							dta.Rank.Find(r => r.Name == ((n as DTB.NodeTree).Nodes[0] as DTB.NodeString).Text).Rank = ((n as DTB.NodeTree).Nodes[1] as DTB.NodeInt32).Number;
							//dta.Rank.Add(new SongsDTA.Rankings() {
							//	Name = ((n as DTB.NodeTree).Nodes[0] as DTB.NodeString).Text,
							//	Rank = ((n as DTB.NodeTree).Nodes[1] as DTB.NodeInt32).Number
							//});
						});
						break;
					case "genre":
						dta.Genre = (subtree.Nodes[1] as DTB.NodeString).Text;
						break;
					case "decade":
						dta.Decade = (subtree.Nodes[1] as DTB.NodeString).Text;
						break;
					case "vocal_gender":
						dta.Vocalist = (subtree.Nodes[1] as DTB.NodeString).Text;
						break;
					case "video_venues":
						(subtree.Nodes[1] as DTB.NodeTree).Nodes.ForEach(n => {
							dta.VideoVenues.Add((n as DTB.NodeString).Text);
						});
						break;
					case "song_practice_1":
						break;
					case "version":
						dta.Version = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "format":
						dta.Format = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "album_art":
						if (subtree.Nodes[1] is DTB.NodeInt32)
							dta.AlbumArt = (subtree.Nodes[1] as DTB.NodeInt32).Number == 1;
						else
							dta.AlbumArt = (subtree.Nodes[1] as DTB.NodeString).Text == "TRUE";
						break;
					case "downloaded":
						if (subtree.Nodes[1] is DTB.NodeInt32)
							dta.Downloaded = (subtree.Nodes[1] as DTB.NodeInt32).Number == 1;
						else
							dta.Downloaded = (subtree.Nodes[1] as DTB.NodeString).Text == "TRUE";
						break;
					case "exported":
						if (subtree.Nodes[1] is DTB.NodeInt32)
							dta.Exported = (subtree.Nodes[1] as DTB.NodeInt32).Number == 1;
						else
							dta.Exported = (subtree.Nodes[1] as DTB.NodeString).Text == "TRUE";
						break;
					case "year_released":
						dta.Year = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "album_name":
						dta.Album = (subtree.Nodes[1] as DTB.NodeString).Text;
						break;
					case "album_track_number":
						dta.Track = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "pack_name":
						dta.Pack = (subtree.Nodes[1] as DTB.NodeString).Text;
						break;
					case "base_points":
						dta.BasePoints = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "rating":
						dta.Rating = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "sub_genre":
						dta.SubGenre = (subtree.Nodes[1] as DTB.NodeString).Text;
						break;
					case "song_id":
						dta.SongID = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "game_origin":
						dta.Origin = (subtree.Nodes[1] as DTB.NodeString).Text;
						break;
					case "ugc":
						dta.Ugc = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
					case "song_length":
						dta.SongLength = (subtree.Nodes[1] as DTB.NodeInt32).Number;
						break;
				}
			}

			if (dta.SongScrollSpeed == 0)
				dta.SongScrollSpeed = 2300;
			if (dta.Song.HopoThreshold.HasValue && dta.Song.HopoThreshold == 0)
				dta.Song.HopoThreshold = null;

			return dta;
		}

		public void Save(Stream stream)
		{
			Save(new StreamWriter(stream));
		}

		public void Save(StreamWriter writer)
		{
			bool first;
			string indent = "   ";
			writer.NewLine = "\r\n";

			writer.WriteLine("(");
			writer.Write(indent); writer.WriteLine("'" + BaseName + "'");

			writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.WriteLine("'name'");
			writer.Write(indent); writer.Write(indent); writer.WriteLine("\"" + Name + "\"");
			writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.WriteLine("'artist'");
			writer.Write(indent); writer.Write(indent); writer.WriteLine("\"" + Artist + "\"");
			writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.WriteLine("('master' " + (Master ? "1" : "0") + ")");

			writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.WriteLine("'song'");

			writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'name'");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("\"" + Song.Name + "\"");
			writer.Write(indent); writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'tracks_count'");
			writer.Write(indent); writer.Write(indent); writer.Write(indent + "("); first = true; foreach (int t in Song.TracksCount) { writer.Write((first ? "" : " ") + t.ToString()); first = false; } writer.WriteLine(")");
			writer.Write(indent); writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'tracks'");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			foreach (var track in Song.Tracks) {
				writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
				writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'" + track.Name + "'");
				writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.Write(indent + "("); first = true; foreach (int t in track.Tracks) { writer.Write((first ? "" : " ") + t.ToString(CultureInfo.InvariantCulture)); first = false; } writer.WriteLine(")");
				writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine(")");
			}
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine(")");
			writer.Write(indent); writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'pans'");
			writer.Write(indent); writer.Write(indent); writer.Write(indent + "("); first = true; foreach (float t in Song.Pans) { writer.Write((first ? "" : " ") + t.ToString("0.00", CultureInfo.InvariantCulture)); first = false; } writer.WriteLine(")");
			writer.Write(indent); writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'vols'");
			writer.Write(indent); writer.Write(indent); writer.Write(indent + "("); first = true; foreach (float t in Song.Vols) { writer.Write((first ? "" : " ") + t.ToString("0.00", CultureInfo.InvariantCulture)); first = false; } writer.WriteLine(")");
			writer.Write(indent); writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'cores'");
			writer.Write(indent); writer.Write(indent); writer.Write(indent + "("); first = true; foreach (int t in Song.Cores) { writer.Write((first ? "" : " ") + t.ToString(CultureInfo.InvariantCulture)); first = false; } writer.WriteLine(")");
			writer.Write(indent); writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'drum_solo'");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'seqs'");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.Write(indent + "("); first = true; foreach (string t in Song.DrumSolo) { writer.Write((first ? "" : " ") + "'" + t + "'"); first = false; } writer.WriteLine(")");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine(")");
			writer.Write(indent); writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'drum_freestyle'");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'seqs'");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.Write(indent + "("); first = true; foreach (string t in Song.DrumFreestyle) { writer.Write((first ? "" : " ") + "'" + t + "'"); first = false; } writer.WriteLine(")");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine(")");
			writer.Write(indent); writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("'midi_file'");
			writer.Write(indent); writer.Write(indent); writer.Write(indent); writer.WriteLine("\"" + Song.MidiFile + "\"");
			writer.Write(indent); writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.WriteLine("('song_scroll_speed' " + SongScrollSpeed.ToString(CultureInfo.InvariantCulture) + ")");

			writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.WriteLine("'bank'");
			writer.Write(indent); writer.Write(indent); writer.WriteLine("\"" + Bank + "\"");
			writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.WriteLine("('anim_tempo' " + AnimTempo.ToString(CultureInfo.InvariantCulture) + ")");
			writer.Write(indent); writer.WriteLine("('song_length' " + SongLength.Value.ToString(CultureInfo.InvariantCulture) + ")");
			writer.Write(indent); writer.WriteLine("('preview' " + Preview[0].ToString(CultureInfo.InvariantCulture) + " " + Preview[1].ToString(CultureInfo.InvariantCulture) + ")");

			writer.Write(indent); writer.WriteLine("(");
			writer.Write(indent); writer.Write(indent); writer.WriteLine("'rank'");
			foreach (var rank in Rank) {
				writer.Write(indent); writer.Write(indent); writer.WriteLine("('" + rank.Name + "' " + rank.Rank.ToString(CultureInfo.InvariantCulture) + ")");
			}
			writer.Write(indent); writer.WriteLine(")");

			writer.Write(indent); writer.WriteLine("('genre' '" + Genre + "')");
			writer.Write(indent); writer.WriteLine("('decade' '" + Decade + "')");
			writer.Write(indent); writer.WriteLine("('vocal_gender' '" + Vocalist + "')");
			writer.Write(indent); writer.WriteLine("('version' " + Version.ToString(CultureInfo.InvariantCulture) + ")");
			writer.Write(indent); writer.WriteLine("('downloaded' " + (Downloaded ? "1" : "0") + ")");
			writer.Write(indent); writer.WriteLine("('format' " + Format.ToString(CultureInfo.InvariantCulture) + ")");
			writer.Write(indent); writer.WriteLine("('album_art' " + (AlbumArt.Value ? "1" : "0") + ")");
			writer.Write(indent); writer.WriteLine("('year_released' " + Year.ToString(CultureInfo.InvariantCulture) + ")");
			writer.Write(indent); writer.WriteLine("('base_points' " + BasePoints.ToString(CultureInfo.InvariantCulture) + ")");
			writer.Write(indent); writer.WriteLine("('rating' " + Rating.Value.ToString(CultureInfo.InvariantCulture) + ")");
			writer.Write(indent); writer.WriteLine("('sub_genre' '" + SubGenre + "')");
			writer.Write(indent); writer.WriteLine("('song_id' " + SongID.Value.ToString(CultureInfo.InvariantCulture) + ")");
			writer.Write(indent); writer.WriteLine("('tuning_offset_cents' " + TuningOffsetCents.Value.ToString("0.00", CultureInfo.InvariantCulture) + ")");
			writer.Write(indent); writer.WriteLine("('context' " + Context.ToString(CultureInfo.InvariantCulture) + ")");
			writer.Write(indent); writer.WriteLine("('game_origin' '" + Origin + "')");
			writer.Write(indent); writer.WriteLine("('ugc' " + Ugc.Value.ToString(CultureInfo.InvariantCulture) + ")");
			writer.WriteLine(")");

			writer.Flush();
		}

		public static SongsDTA Create(Stream stream)
		{
			SongsDTA dta = new SongsDTA();

			StreamReader reader = new StreamReader(stream, Encoding.GetEncoding(28591));

			while (!reader.EndOfStream) {
				string line = reader.ReadLine();
				line = line.Trim();
				if (line.StartsWith("(name \"")) {
					dta.Name = Regex.Match(line, Regex.Escape("(name \"") + "(?'name'.*?)\"").Groups["name"].Value;
				} else if (line.StartsWith("(artist")) {
					dta.Artist = Regex.Match(line, Regex.Escape("(artist \"") + "(?'artist'.*?)\"").Groups["artist"].Value;
				} else if (line.StartsWith("(master")) {
					dta.Master = Regex.Match(line, @"\(master (?'master'.*?)\)").Groups["master"].Value == "TRUE";
				} else if (line.StartsWith("(genre")) {
					dta.Genre = Regex.Match(line, @"\(genre (?'genre'.*?)\)").Groups["genre"].Value;
				} else if (line.StartsWith("(vocal_gender")) {
					dta.Vocalist = Regex.Match(line, @"\(vocal_gender (?'gender'.*?)\)").Groups["gender"].Value;
				} else if (line.StartsWith("(year_released")) {
					dta.Year = ushort.Parse(Regex.Match(line, @"\(year_released (?'year'.*?)\)").Groups["year"].Value);
				} else if (line.StartsWith("(album_name")) {
					dta.Album = Regex.Match(line, Regex.Escape("(album_name \"") + "(?'album'.*?)\"").Groups["album"].Value;
				} else if (line.StartsWith("(album_track_number")) {
					dta.Track = ushort.Parse(Regex.Match(line, @"\(album_track_number (?'track'.*?)\)").Groups["track"].Value);
				} else if (line.StartsWith("(pack_name")) {
					dta.Pack = Regex.Match(line, Regex.Escape("(pack_name \"") + "(?'pack'.*?)\"").Groups["pack"].Value;
				}
			}

			return dta;
		}

		public class NameComparer : IComparer<SongsDTA>
		{
			public int Compare(SongsDTA x, SongsDTA y)
			{
				return x.Name.CompareTo(y.Name);
			}
		}
	}
}
