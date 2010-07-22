using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;

namespace AmplitudeSongExtractor
{
	class Program
	{
		static void Main(string[] args)
		{
			string arkpath = args[0];
			string songname = args[1];

			FileStream arkfile = new FileStream(arkpath, FileMode.Open, FileAccess.Read);

			Ark ark = new Ark(new EndianReader(arkfile, Endianness.LittleEndian));

			DirectoryNode songdir = ark.Root.Find("songs") as DirectoryNode;
			songdir = songdir.Find(songname) as DirectoryNode;

			Midi midi = Midi.Create(Mid.Create((songdir.Find(songname + "_g.mid") as FileNode).Data));

			Dictionary<InstrumentBank, Stream> banks = new Dictionary<InstrumentBank, Stream>();
			foreach (FileNode node in songdir.Files) {
				if (node.Name.EndsWith(".bnk")) {
					InstrumentBank bank = InstrumentBank.Create(new EndianReader(node.Data, Endianness.LittleEndian));

					FileNode nse = songdir.Find(Path.GetFileNameWithoutExtension(node.Name) + ".nse") as FileNode;

					banks.Add(bank, nse.Data);
				}
			}

			int tracknum = 0;
			FileStream outfile = new FileStream(@"Z:\" + songname + "_" + midi.Tracks.Count.ToString() + ".raw", FileMode.Create, FileAccess.ReadWrite);
			//EndianReader writer = new EndianReader(outfile, Endianness.BigEndian);
			EndianReader writer = new EndianReader(new TemporaryStream(), Endianness.BigEndian);
			foreach (Midi.Track track in midi.Tracks) {
				List<Midi.Event> events = new List<Midi.Event>();
				events.AddRange(track.Banks.Cast<Midi.Event>());
				events.AddRange(track.Instruments.Cast<Midi.Event>());
				events.AddRange(track.Notes.Cast<Midi.Event>());

				InstrumentBank[] bankids = new InstrumentBank[0x10];
				InstrumentBank.Bank[] subbankids = new InstrumentBank.Bank[0x10];
				InstrumentBank.Instrument[] instrumentids = new InstrumentBank.Instrument[0x10];
				InstrumentBank.Sound[][] soundids = new InstrumentBank.Sound[0x10][];

				events.Sort(new SpecialEventComparer());

				foreach (Midi.ChannelEvent e in events.Cast<Midi.ChannelEvent>()) {
					Midi.BankEvent banke = e as Midi.BankEvent;
					Midi.InstrumentEvent instrumente = e as Midi.InstrumentEvent;
					Midi.NoteEvent notee = e as Midi.NoteEvent;

					if (banke != null) {
						bankids[banke.Channel] = banks.Select(b => b.Key).SingleOrDefault(b => b.Banks.SingleOrDefault(b2 => b2.ID == banke.Bank) != null);
						subbankids[banke.Channel] = bankids[banke.Channel].Banks.SingleOrDefault(b => b.ID == banke.Bank);
					} else if (instrumente != null) {
						instrumentids[instrumente.Channel] = bankids[instrumente.Channel].Instruments.SingleOrDefault(i => i.ID == instrumente.Instrument);
						int soundoffset = 0;
						foreach (var instrument in bankids[instrumente.Channel].Instruments) {
							if (instrument == instrumentids[instrumente.Channel])
								break;

							soundoffset += instrument.Sounds;
						}
						soundids[instrumente.Channel] = bankids[instrumente.Channel].Sounds.Skip(soundoffset).Take(instrumentids[instrumente.Channel].Sounds).ToArray();
					} else {
						var bank = bankids[notee.Channel];
						if (bank == null)
							continue;

						var instrument = instrumentids[notee.Channel];
						if (instrument == null)
							continue; // Chart note, not audio-playable

						var sound = soundids[notee.Channel].FirstOrDefault(s => s.ID0 == notee.Note); // Should be SingleOrDefault, but some duplicates use Sound.Unknown
						if (sound == null)
							continue;

						ulong notetime = midi.GetTime(notee.Time);
						long sample = (long)(notetime / 1000 * (uint)bank.Samples[sound.Sample].SampleRate / 1000);
						long duration = (long)((midi.GetTime(notee.Time + notee.Duration) - notetime) / 1000 * (uint)bank.Samples[sound.Sample].SampleRate / 1000);

						short[] samples = bank.Decode(banks[bank], bank.Samples[sound.Sample]);
						
						int volume = sound.Volume * notee.Velocity * instrument.Volume * subbankids[notee.Channel].Volume / 0x7F / 0x7F / 0x7F;
						//int balance = ((int)sound.Balance - 0x40) * ((int)instrument.Balance - 0x40) / 0x40;
						int balance = (int)sound.Balance - 0x40;
						int lvolume = balance <= 0x00 ? 0x7F : 0x7F - balance * 2;
						int rvolume = balance >= 0x00 ? 0x7F : 0x7F + balance * 2;

						lvolume = lvolume * volume / 0x7F;
						rvolume = rvolume * volume / 0x7F;

						writer.Position = (sample * 2) * midi.Tracks.Count * 2 + tracknum * 2 * 2;
						//writer.Position = (sample * 2) * midi.Tracks.Count + tracknum * 2;
						duration = Math.Min(duration, samples.Length);
						for (int i = 0; i < duration; i++) {
							//writer.Write(samples[i]);
							
							if (lvolume > 1)
								writer.Write((short)((int)samples[i] * lvolume / 0x7F));
							else
								writer.Position += 2;
							if (rvolume > 1)
								writer.Write((short)((int)samples[i] * rvolume / 0x7F));
							else
								writer.Position += 2;

							writer.Position += 2 * (midi.Tracks.Count - 1) * 2;
							
							//writer.Position += 2 * (midi.Tracks.Count - 1);
						}
					}
				}

				tracknum++;
				Console.WriteLine("Track: " + tracknum.ToString());
			}

			writer.Position = 0;
			Util.StreamCopy(outfile, writer.Base);

			outfile.Close();
			writer.Base.Close();

			arkfile.Close();
		}

		class SpecialEventComparer : IComparer<Midi.Event>
		{
			public int Compare(Midi.Event x, Midi.Event y)
			{
				int ret = x.Time.CompareTo(y.Time);
				if (ret == 0) {
					if (x is Midi.BankEvent && !(y is Midi.BankEvent))
						return -1;
					if (y is Midi.BankEvent && !(x is Midi.BankEvent))
						return 1;
					if (x is Midi.InstrumentEvent && !(y is Midi.InstrumentEvent))
						return -1;
					if (y is Midi.InstrumentEvent && !(x is Midi.InstrumentEvent))
						return 1;
				}
				return ret;
			}
		}
	}
}
