
#define DISC_RB1			0
#define DISC_TP1			1
#define DISC_TP2			2
#define DISC_ACDC			3
#define DISC_CLASSIC		4
#define DISC_COUNTRY		5
#define DISC_GHWT			6
#define DISC_GH3			7
#define DISC_GHM			8
#define DISC_GHSH			9
#define DISC_GHA			10
#define DISC_GH2			11
#define DISC_GH1			12
#define DISC_GH80S			13
#define DISC_GH5			14
#define DISC_METAL			15
#define DISC_GDRB			16
#define DISC_BH             17
#define DISC_GHVH           18
#define DISC_LRB			19

#define DISC_TBRB			253
#define DISC_RB2			254
#define DISC_UNRECOGNIZED	255

#define MIN_FREE_SPACE		16384

#define DEVICE_FULL			-1
#define READ_ERROR			-2
#define WRITE_ERROR			-3
#define BEGIN_ERROR			-4

#define DEVICE_NONE			-1

int get_disc();
void close_disc();
u64 get_space(int device, int init=1);

struct rip_state {
	FILE *f_in;
	s32 f_out;
	u64 current;
	u64 total;
	const char* disc_name;
	char status_text[300];
	char file_name[32];
	int out_device;
	int disc;
	u64 bytes_in_current_file;
	u64 offset_in_current_file;
	char dir_name[50];
	DIR_ITER *d;
	int file_index;
	time_t start;
	time_t switch_time;
	u64 device_free_space;
	const char *out_filename;
};

u64 begin_rip(struct rip_state *rs,int disc);
s64 process_rip(struct rip_state *rs);
void end_rip(struct rip_state *rs);
bool prepare_device(struct rip_state *rs, int device);
