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
		file_data = mmap(0, file_size, PROT_READ, MAP_PRIVATE, input_file_fd, 0);
		close(input_file_fd);
	}
	return (struct file_content){file_data, file_size};
}

struct message_info {
	u8 *pixels;
	u32 msg_len;
	u32 msg_start;
	u32 width;
};

u32 find_header_start(u8 *data, u32 size, u32 offset, u32 width) {
	(void) width;
	for (u32 j = offset; j < size; j += 4 * 7) {
		if (
			data[j] == 127 && data[j + 1] == 188 && data[j + 2] == 217
		) {
			for (u32 i = j - 4; i < j + 4; i += 4) {
				if (data[i] == 127 && data[i + 1] == 188 && data[i + 2] == 217 &&
					data[i + 4] == 127 && data[i + 5] == 188 && data[i + 6] == 217 &&
				data[i + 8] == 127 && data[i + 9] == 188 && data[i + 10] == 217 &&
				data[i + 12] == 127 && data[i + 13] == 188 && data[i + 14] == 217 &&
				data[i + 16] == 127 && data[i + 17] == 188 && data[i + 18] == 217 &&
				data[i + 20] == 127 && data[i + 21] == 188 && data[i + 22] == 217 &&
				data[i + 24] == 127 && data[i + 25] == 188 && data[i + 26] == 217)  {
					return i;
				} else {
					continue;
				}
			}
		}
		// 	data[i + 4] == 127 && data[i + 5] == 188 && data[i + 6] == 217 &&
		// 	data[i + 8] == 127 && data[i + 9] == 188 && data[i + 10] == 217 &&
		// 	data[i + 12] == 127 && data[i + 13] == 188 && data[i + 14] == 217 &&
		// 	data[i + 16] == 127 && data[i + 17] == 188 && data[i + 18] == 217 &&
		// 	data[i + 20] == 127 && data[i + 21] == 188 && data[i + 22] == 217 &&
		// 	data[i + 24] == 127 && data[i + 25] == 188 && data[i + 26] == 217
		// 	&& data[i - width * 4] == 127 && data[i - width * 4 + 1] == 188 && data[i - width * 4 + 2] == 217
		// 	&& data[i - width * 4 * 2] == 127 && data[i - width * 4 * 2 + 1] == 188 && data[i - width * 4 * 2 + 2] == 217
		// 	&& data[i - width * 4 * 3] == 127 && data[i - width * 4 * 3 + 1] == 188 && data[i - width * 4 * 3 + 2] == 217
		// 	&& data[i - width * 4 * 4] == 127 && data[i - width * 4 * 4 + 1] == 188 && data[i - width * 4 * 4 + 2] == 217
		// 	&& data[i - width * 4 * 5] == 127 && data[i - width * 4 * 5 + 1] == 188 && data[i - width * 4 * 5 + 2] == 217
		// 	&& data[i - width * 4 * 6] == 127 && data[i - width * 4 * 6 + 1] == 188 && data[i - width * 4 * 6 + 2] == 217
		// 	&& data[i - width * 4 * 7] == 127 && data[i - width * 4 * 7 + 1] == 188 && data[i - width * 4 * 7 + 2] == 217
		// ) 
		// {
		// 	return i;
		// }
	}
	return 0;
}

// TODO: optimize LAST (only 510 chars max)
void print_msg(struct message_info msg_info) {
	u32 pos = msg_info.msg_start;
	u32 chars_read = 0;
	while (msg_info.msg_len - chars_read >= 3) {
		write(STDOUT_FILENO, msg_info.pixels + pos, 3);
		chars_read += 3;
		pos += 4;
		if (chars_read % 18 == 0) {
			pos -= msg_info.width * 4;
			pos -= 6 * 4;
		}
	}
	if (msg_info.msg_len - chars_read > 0) {
		write(STDOUT_FILENO, msg_info.pixels + pos, msg_info.msg_len - chars_read);
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
	u32 header_start = find_header_start((u8*)file_content.data, file_content.size, header->data_offset, header->width);
	struct message_info msg_info;
	u32 msg_len_pos = header_start + 7 * 4;
	msg_info.pixels = (u8*)file_content.data;
	msg_info.msg_len = msg_info.pixels[msg_len_pos] + msg_info.pixels[msg_len_pos + 2];
	msg_info.width = header->width;
	msg_info.msg_start = header_start - header->width * 2 * 4 + 2 * 4;
	print_msg(msg_info);
	return 0;
}
