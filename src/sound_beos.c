#include <Application.h>
#include <SoundPlayer.h>

#define B_AUDIO_CHAR 1
#define B_AUDIO_SHORT 2

extern "C" {
#include <string.h>
#include <stdlib.h>
#include "xmp.h"
#include "sound.h"
}

static media_raw_audio_format fmt;
static BSoundPlayer *player;


/*
 * CoreAudio helpers from mplayer/libao
 * The player fills a ring buffer, BSP retrieves data from the buffer
 */

static int paused;
static uint8 *buffer;
static int buffer_len;
static int buf_write_pos;
static int buf_read_pos;
static int chunk_size;
static int chunk_num;
/* static int packet_size; */


static char *help[] = {
	"buffer=num,size", "set the number and size of buffer fragments",
	NULL
};

static int init(struct options *options);
static void deinit();
static void play(void *b, int i);
static void flush();
static void onpause();
static void onresume();

struct sound_driver sound_beos = {
	"beos",
	"BeOS PCM audio",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};

/* return minimum number of free bytes in buffer, value may change between
 * two immediately following calls, and the real number of free bytes
 * might actually be larger!  */
static int buf_free()
{
	int free = buf_read_pos - buf_write_pos - chunk_size;
	if (free < 0)
		free += buffer_len;
	return free;
}

/* return minimum number of buffered bytes, value may change between
 * two immediately following calls, and the real number of buffered bytes
 * might actually be larger! */
static int buf_used()
{
	int used = buf_write_pos - buf_read_pos;
	if (used < 0)
		used += buffer_len;
	return used;
}

/* add data to ringbuffer */
static int write_buffer(unsigned char *data, int len)
{
	int first_len = buffer_len - buf_write_pos;
	int free = buf_free();

	if (len > free)
		len = free;
	if (first_len > len)
		first_len = len;

	/* till end of buffer */
	memcpy(buffer + buf_write_pos, data, first_len);
	if (len > first_len) {	/* we have to wrap around */
		/* remaining part from beginning of buffer */
		memcpy(buffer, data + first_len, len - first_len);
	}
	buf_write_pos = (buf_write_pos + len) % buffer_len;

	return len;
}

/* remove data from ringbuffer */
static int read_buffer(unsigned char *data, int len)
{
	int first_len = buffer_len - buf_read_pos;
	int buffered = buf_used();

	if (len > buffered)
		len = buffered;
	if (first_len > len)
		first_len = len;

	/* till end of buffer */
	memcpy(data, buffer + buf_read_pos, first_len);
	if (len > first_len) {	/* we have to wrap around */
		/* remaining part from beginning of buffer */
		memcpy(data + first_len, buffer, len - first_len);
	}
	buf_read_pos = (buf_read_pos + len) % buffer_len;

	return len;
}

/*
 * end of CoreAudio helpers
 */


void render_proc(void *theCookie, void *buffer, size_t req, 
				const media_raw_audio_format &format)
{ 
        size_t amt;

	while ((amt = buf_used()) < req)
		snooze(100000);

        read_buffer((unsigned char *)buffer, req);
}


static int init(struct options *options)
{
	char **parm = options->driver_parm;
	static char desc[80];

	be_app = new BApplication("application/x-vnd.cm-xmp");

	chunk_size = 4096;
	chunk_num = 20;

	parm_init(parm);
	chkparm2("buffer", "%d,%d", &chunk_num, &chunk_size);
	parm_end();

	snprintf(desc, 80, "%s [%d fragments of %d bytes]",
			sound_beos.description, chunk_num, chunk_size);
	sound_beos.description = desc;

	fmt.frame_rate = options->rate;
	fmt.channel_count = options->format & XMP_MIX_MONO ? 1 : 2;
	fmt.format = options->format & XMP_MIX_8BIT ?
				B_AUDIO_CHAR : B_AUDIO_SHORT;
	fmt.byte_order = B_HOST_IS_LENDIAN ?
				B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
	fmt.buffer_size = chunk_size * chunk_num;

	buffer_len = chunk_size * chunk_num;
	buffer = (uint8 *)calloc(1, buffer_len);
	buf_read_pos = 0;
	buf_write_pos = 0;
	paused = 1;
	
	player = new BSoundPlayer(&fmt, "xmp output", render_proc);

	return 0;
}


static void play(void *b, int i)
{
	int j = 0;

	/* block until we have enough free space in the buffer */
	while (buf_free() < i)
		snooze(100000);

	while (i) {
        	if ((j = write_buffer((uint8 *)b, i)) > 0) {
			i -= j;
			b = (uint8 *)b + j;
		} else
			break;
	}

	if (paused) {
		player->Start(); 
		player->SetHasData(true);
		paused = 0;
	}
}

static void deinit()
{
	player->Stop(); 
	be_app->Lock();
	be_app->Quit();
}

static void flush()
{
}

static void onpause()
{
}

static void onresume()
{
}

