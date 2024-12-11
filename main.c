#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

typedef char i8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef int i32;
typedef unsigned u32;
typedef unsigned long u64;

#define PRINT_ERROR(cstring) write(STDERR_FILENO, cstring, sizeof(cstring) - 1)

#pragma pack(1)
struct bmp_header
{
	// Note: header
	i8 signature[2]; // should equal to "BM"
	u32 file_size;
	u32 unused_0;
	u32 data_offset;

	// Note: info header
	u32 info_header_size;
	u32 width;				   // in px
	u32 height;				   // in px
	u16 number_of_planes;	   // should be 1
	u16 bit_per_pixel;		   // 1, 4, 8, 16, 24 or 32
	u32 compression_type;	   // should be 0
	u32 compressed_image_size; // should be 0
							   // Note: there are more stuff there but it is not important here
};

struct file_content
{
	i8 *data;
	u32 size;
};

struct file_content read_entire_file(char *filename)
{
	char *file_data = 0;
	unsigned long file_size = 0;
	int input_file_fd = open(filename, O_RDONLY);
	if (input_file_fd >= 0)
	{
		struct stat input_file_stat = {0};
		stat(filename, &input_file_stat);
		file_size = input_file_stat.st_size;
		file_data = mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, input_file_fd, 0);
		close(input_file_fd);
	}
	return (struct file_content){file_data, file_size};
}

struct message_info {
	u8 *pixels;
	// u32 header_pos;
	u32 msg_len;
	u32 msg_start;
	u32 width;
};

// struct message_info get_msg_info() {
	// maybe juts take position of the header (finding header is the most complex part)
// } 

void print_msg(struct message_info msg_info) {
	u32 pos = msg_info.msg_start;
	u32 chars_read = 0;
	while (chars_read - msg_info.msg_len >= 3) {
		write(STDOUT_FILENO, msg_info.pixels + pos, 3);
		chars_read += 3;
		pos += 4;
		if (chars_read % 18 == 0) {
			pos -= msg_info.width * 4;
			pos -= 6 * 4;
		}
	}
}


int main(int argc, char **argv)
{
	if (argc != 2)
	{
		PRINT_ERROR("Usage: decode <input_filename>\n");
		return 1;
	}
	struct file_content file_content = read_entire_file(argv[1]);
	if (file_content.data == NULL)
	{
		PRINT_ERROR("Failed to read file\n");
		return 1;
	}
	struct bmp_header *header = (struct bmp_header *)file_content.data;
	printf("signature: %.2s\nfile_size: %u\ndata_offset: %u\ninfo_header_size: %u\nwidth: %u\nheight: %u\nplanes: %i\nbit_per_px: %i\ncompression_type: %u\ncompression_size: %u\n", header->signature, header->file_size, header->data_offset, header->info_header_size, header->width, header->height, header->number_of_planes, header->bit_per_pixel, header->compression_type, header->compressed_image_size);
	for (u32 i = header->data_offset; i < file_content.size; i += 4) {
		u8 blue = file_content.data[i];
		u8 green = file_content.data[i + 1];
		u8 red = file_content.data[i + 2];
		if (blue == 127 && green == 188 && red == 217) {
			printf("found: %u\n", i);
		}
	}
	// u8 *pixels = (u8*)file_content.data;
	// u32 msg_len_pos = 4074562 + 4;
	// u32 msg_len_pos = 48290162 + 4;
	// u32 msg_len = pixels[msg_len_pos] + pixels[msg_len_pos + 2];
	// printf("len: %u\n", msg_len);

	// u32 pos = 48257946 + 8;
	// u32 pixel_msg_len = (msg_len + 2) / 3;
	// for (u32 i = 0; i < pixel_msg_len; i++) {
	// 	write(STDOUT_FILENO, pixels + pos, 1);
	// 	write(STDOUT_FILENO, pixels + pos + 1, 1);
	// 	write(STDOUT_FILENO, pixels + pos + 2, 1);
	// 	if (i % 6 == 5) {
	// 		pos -= header->width * 4;
	// 		pos -= 5 * 4;
	// 	} else {
	// 		pos += 1 * 4;
	// 	}
	// }
	struct message_info msg_info;
	msg_info.pixels = (u8*)file_content.data;
	msg_info.msg_len = 396; // chars
	msg_info.msg_start = 48257946 + 8;
	msg_info.width = header->width;
	print_msg(msg_info);
	return 0;
}
