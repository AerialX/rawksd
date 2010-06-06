using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Qyoto;

namespace ConsoleHaxx.RawkSD.Qyoto
{
	partial class MainForm
	{
		public QTreeView SongList;

		private void Initialise()
		{
			SetWindowTitle("RawkSD");

			SongList = new QTreeView();
			SetCentralWidget(SongList);
			QStandardItemModel songmodel = new QStandardItemModel(0, 3);
			songmodel.SetHeaderData(0, Orientation.Horizontal, "Song");
			songmodel.SetHeaderData(1, Orientation.Horizontal, "Artist");
			songmodel.SetHeaderData(2, Orientation.Horizontal, "etc");
			SongList.SetModel(songmodel);
		}
	}
}
