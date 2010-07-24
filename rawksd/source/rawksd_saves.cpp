#include "rawksd_menu.h"

#include "files.h"

extern "C" {
	extern const u8 menu_backup_png[];
	extern const u8 menu_backup_sel_png[];
	extern const u8 menu_restore_png[];
	extern const u8 menu_restore_sel_png[];
}

const u8 *MenuSaves::saves_images[OPTION_SAVES_COUNT*3+1] = {
	menu_backup_png,
	menu_backup_sel_png,
	NULL,
	menu_restore_png,
	menu_restore_sel_png,
	NULL,
	NULL
};

MenuSaves::MenuSaves(GuiWindow *Parent) : RawkMenu(Parent, saves_images, 238, 188),
Main(NULL)
{
	MenuButton *back = new MenuButton(Parent, 0, 0, NULL, Triggers::Back, 2);
	back->button->SetSelectable(false);
	Buttons.push_back(back);
}

MenuSaves::MenuSaves(GuiWindow *Parent, const char *title, const char *text, int cont) :
RawkMenu(cont ? popup_error : NULL, text ? text : "\nSavegame management requires a storage device to be connected to your wii.", \
		 title ? title : "No Storage Device Found"),
Main(Parent)
{
}

u8 save_copy_buffer[0x8000] ATTRIBUTE_ALIGN(32);

RawkMenu* MenuSaves::Process()
{
	int clicked = GetClicked();
	if (clicked<0 && (Main || default_mount>=0))
		return this;

	HaltGui();
	if (Main)
		return new MenuMain(Main);
	if (clicked==2)
		return new MenuMain(Parent);

	int succeeded;
	File_SetDefault(default_mount);
	HaltGui();
	MenuSaves *copying = new MenuSaves(Parent, "Please Wait", "\nCopying in progress....", 0);
	ResumeGui();

	if (clicked==OPTION_BACKUP) {
		succeeded=1;
		File_CreateDir("/rawk");
		File_CreateDir("/rawk/rb2");
		s32 in_fd = File_Open("/mnt/isfs/title/00010000/535a4145/data/rockbnd2.dat", O_RDONLY);
		if (in_fd>=0) {
			s32 out_fd;
			File_CreateDir("/rawk/rb2/ntsc");
			out_fd = File_Open("/rawk/rb2/ntsc/rockbnd2.dat", O_WRONLY|O_CREAT|O_TRUNC);
			if (out_fd<0)
				succeeded = -1;
			else {
				int readed;
				copying->popup_text[0]->SetText("Found NTSC savefile");
				while ((readed = File_Read(in_fd, save_copy_buffer, sizeof(save_copy_buffer)))>0) {
					int wrote = File_Write(out_fd, save_copy_buffer, readed);
					if (wrote != readed) {
						succeeded = -1;
						break;
					}
				}
				File_Close(out_fd);
			}
			File_Close(in_fd);
		} else
			succeeded = -2;
		// PAL
		if (succeeded != -1) {
			in_fd = File_Open("/mnt/isfs/title/00010000/535a4150/data/rockbnd2.dat", O_RDONLY);
			if (in_fd>=0) {
				succeeded += 2;
				s32 out_fd;
				File_CreateDir("/rawk/rb2/pal");
				out_fd = File_Open("/rawk/rb2/pal/rockbnd2.dat", O_WRONLY|O_CREAT|O_TRUNC);
				if (out_fd<0)
					succeeded = -1;
				else {
					int readed;
					copying->popup_text[0]->SetText("Found PAL savefile");
					while ((readed = File_Read(in_fd, save_copy_buffer, sizeof(save_copy_buffer)))>0) {
						int wrote = File_Write(out_fd, save_copy_buffer, readed);
						if (wrote != readed) {
							succeeded = -1;
							break;
						}
					}
					File_Close(out_fd);
				}
				File_Close(in_fd);
			}
		}
	}
	else { // OPTION_RESTORE
		succeeded=1;
		s32 in_fd = File_Open("/rawk/rb2/ntsc/rockbnd2.dat", O_RDONLY);
		if (in_fd>=0) {
			s32 out_fd;
			out_fd = File_Open("/mnt/isfs/title/00010000/535a4145/data/rockbnd2.dat", O_WRONLY);
			if (out_fd<0)
				succeeded = -4;
			else {
				int readed;
				copying->popup_text[0]->SetText("Found NTSC savefile");
				while ((readed = File_Read(in_fd, save_copy_buffer, sizeof(save_copy_buffer)))>0) {
					int wrote = File_Write(out_fd, save_copy_buffer, readed);
					if (wrote != readed) {
						succeeded = -3;
						break;
					}
				}
				File_Close(out_fd);
			}
			File_Close(in_fd);
		} else
			succeeded = -2;
		// PAL
		if (succeeded > -3) {
			in_fd = File_Open("/rawk/rb2/pal/rockbnd2.dat", O_RDONLY);
			if (in_fd>=0) {
				succeeded += 2;
				s32 out_fd;
				out_fd = File_Open("/mnt/isfs/title/00010000/535a4150/data/rockbnd2.dat", O_WRONLY);
				if (out_fd<0)
					succeeded = -4;
				else {
					int readed;
					copying->popup_text[0]->SetText("Found PAL savefile");
					while ((readed = File_Read(in_fd, save_copy_buffer, sizeof(save_copy_buffer)))>0) {
						int wrote = File_Write(out_fd, save_copy_buffer, readed);
						if (wrote != readed) {
							succeeded = -3;
							break;
						}
					}
					File_Close(out_fd);
				}
				File_Close(in_fd);
			}
		}
	}

	HaltGui();
	delete copying;
	ResumeGui();

	switch(succeeded) {
		case 3:
			return new MenuSaves(Parent, "Copying Done", "\nNTSC and PAL savefiles were copied successfully.");
		case 1:
			return new MenuSaves(Parent, "Copying Done", "\nNTSC savefile was copied successfully.");
		case 0:
			return new MenuSaves(Parent, "Copying Done", "\nPAL savefile was copied successfully.");
		case -1:
			return new MenuSaves(Parent, "Error", "\nCouldn't write to storage device.");
		case -2:
			return new MenuSaves(Parent, "Error", "\nCouldn't find any saves to copy.");
		case -3:
			return new MenuSaves(Parent, "Error", "\nCouldn't write to NAND.\n");
		case -4:
			return new MenuSaves(Parent, "Error", "\nSavefile does not exist on NAND, can't replace it.\n");
		default: {
			char s[50];
			sprintf(s, "\nCouldn't copy savefile (%d)", succeeded);
			return new MenuSaves(Parent, "Error", s); }
	}
}
