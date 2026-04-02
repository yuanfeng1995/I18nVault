// SM4-GCM i18n converter: JSON <-> TRS
#include "sm4_gcm.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <errno.h>
#endif

#define TRS_MAGIC "TRS1"
#define TRS_VERSION 1

typedef struct CliOptions
{
	const char* mode;
	const char* input_path;
	const char* output_path;
	const char* key_hex;
	const char* aad;
	const char* iv_hex;
} CliOptions;

static void print_usage(const char* exe)
{
	fprintf(stderr,
			"Usage:\n"
			"  %s enc -i <input.json> -o <output.trs> -k <32-hex-key> [--aad <text>] [--iv-hex <24-hex-iv>]\n"
			"  %s dec -i <input.trs>  -o <output.json> -k <32-hex-key> [--aad <text>]\n",
			exe, exe);
}

static int hex_nibble(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');
	if (c >= 'A' && c <= 'F')
		return 10 + (c - 'A');
	return -1;
}

static int parse_hex_exact(const char* hex, uint8_t* out, size_t out_len)
{
	size_t hex_len = strlen(hex);
	if (hex_len != out_len * 2)
		return -1;

	for (size_t i = 0; i < out_len; ++i)
	{
		int hi = hex_nibble(hex[2 * i]);
		int lo = hex_nibble(hex[2 * i + 1]);
		if (hi < 0 || lo < 0)
			return -1;
		out[i] = (uint8_t)((hi << 4) | lo);
	}
	return 0;
}

static int read_file_all(const char* path, uint8_t** data, size_t* len)
{
	FILE* f = fopen(path, "rb");
	if (!f)
		return -1;

	if (fseek(f, 0, SEEK_END) != 0)
	{
		fclose(f);
		return -1;
	}

	long sz = ftell(f);
	if (sz < 0)
	{
		fclose(f);
		return -1;
	}
	rewind(f);

	uint8_t* buf = (uint8_t*)malloc((size_t)sz ? (size_t)sz : 1);
	if (!buf)
	{
		fclose(f);
		return -1;
	}

	size_t rd = fread(buf, 1, (size_t)sz, f);
	fclose(f);
	if (rd != (size_t)sz)
	{
		free(buf);
		return -1;
	}

	*data = buf;
	*len = (size_t)sz;
	return 0;
}

static int write_file_all(const char* path, const uint8_t* data, size_t len)
{
	FILE* f = fopen(path, "wb");
	if (!f)
		return -1;

	size_t wr = fwrite(data, 1, len, f);
	fclose(f);
	return wr == len ? 0 : -1;
}

static int write_u32_le(FILE* f, uint32_t v)
{
	uint8_t b[4];
	b[0] = (uint8_t)(v & 0xFF);
	b[1] = (uint8_t)((v >> 8) & 0xFF);
	b[2] = (uint8_t)((v >> 16) & 0xFF);
	b[3] = (uint8_t)((v >> 24) & 0xFF);
	return fwrite(b, 1, sizeof(b), f) == sizeof(b) ? 0 : -1;
}

static uint32_t read_u32_le(const uint8_t b[4])
{
	return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static int fill_random_bytes(uint8_t* out, size_t len)
{
#ifdef _WIN32
	for (size_t i = 0; i < len; ++i)
	{
		unsigned int r = 0;
		if (rand_s(&r) != 0)
			return -1;
		out[i] = (uint8_t)(r & 0xFF);
	}
	return 0;
#else
	FILE* f = fopen("/dev/urandom", "rb");
	if (!f)
		return -1;
	size_t rd = fread(out, 1, len, f);
	fclose(f);
	return rd == len ? 0 : -1;
#endif
}

static int parse_args(int argc, char** argv, CliOptions* opts)
{
	if (argc < 2)
		return -1;

	memset(opts, 0, sizeof(*opts));
	opts->mode = argv[1];
	opts->aad = "";

	for (int i = 2; i < argc; ++i)
	{
		if (strcmp(argv[i], "-i") == 0 && i + 1 < argc)
			opts->input_path = argv[++i];
		else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
			opts->output_path = argv[++i];
		else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc)
			opts->key_hex = argv[++i];
		else if (strcmp(argv[i], "--aad") == 0 && i + 1 < argc)
			opts->aad = argv[++i];
		else if (strcmp(argv[i], "--iv-hex") == 0 && i + 1 < argc)
			opts->iv_hex = argv[++i];
		else
			return -1;
	}

	if (!opts->input_path || !opts->output_path || !opts->key_hex)
		return -1;

	if (strcmp(opts->mode, "enc") != 0 && strcmp(opts->mode, "dec") != 0)
		return -1;

	return 0;
}

static int do_encrypt(const CliOptions* opts)
{
	uint8_t key[SM4_GCM_KEY_SIZE];
	uint8_t iv[SM4_GCM_IV_SIZE];
	uint8_t tag[SM4_GCM_TAG_SIZE];
	uint8_t* pt = NULL;
	uint8_t* ct = NULL;
	size_t pt_len = 0;

	if (parse_hex_exact(opts->key_hex, key, sizeof(key)) != 0)
	{
		fprintf(stderr, "[ERROR] invalid key hex (must be 32 hex chars)\n");
		return 1;
	}

	if (opts->iv_hex)
	{
		if (parse_hex_exact(opts->iv_hex, iv, sizeof(iv)) != 0)
		{
			fprintf(stderr, "[ERROR] invalid iv hex (must be 24 hex chars)\n");
			return 1;
		}
	}
	else if (fill_random_bytes(iv, sizeof(iv)) != 0)
	{
		fprintf(stderr, "[ERROR] failed to generate random iv\n");
		return 1;
	}

	if (read_file_all(opts->input_path, &pt, &pt_len) != 0)
	{
		fprintf(stderr, "[ERROR] cannot read input file: %s\n", opts->input_path);
		return 1;
	}

	ct = (uint8_t*)malloc(pt_len ? pt_len : 1);
	if (!ct)
	{
		fprintf(stderr, "[ERROR] out of memory\n");
		free(pt);
		return 1;
	}

	if (sm4_gcm_encrypt(key, iv, sizeof(iv), (const uint8_t*)opts->aad, strlen(opts->aad), pt, pt_len, ct, tag) != 0)
	{
		fprintf(stderr, "[ERROR] sm4_gcm_encrypt failed\n");
		free(pt);
		free(ct);
		return 1;
	}

	FILE* out = fopen(opts->output_path, "wb");
	if (!out)
	{
		fprintf(stderr, "[ERROR] cannot open output file: %s\n", opts->output_path);
		free(pt);
		free(ct);
		return 1;
	}

	if (fwrite(TRS_MAGIC, 1, 4, out) != 4 || fputc(TRS_VERSION, out) == EOF || fputc(SM4_GCM_IV_SIZE, out) == EOF
		|| fputc(SM4_GCM_TAG_SIZE, out) == EOF || fputc(0, out) == EOF || write_u32_le(out, (uint32_t)pt_len) != 0
		|| fwrite(iv, 1, sizeof(iv), out) != sizeof(iv) || fwrite(tag, 1, sizeof(tag), out) != sizeof(tag)
		|| fwrite(ct, 1, pt_len, out) != pt_len)
	{
		fclose(out);
		fprintf(stderr, "[ERROR] failed to write TRS file\n");
		free(pt);
		free(ct);
		return 1;
	}

	fclose(out);
	free(pt);
	free(ct);
	fprintf(stdout, "[OK] encrypted to %s\n", opts->output_path);
	return 0;
}

static int do_decrypt(const CliOptions* opts)
{
	uint8_t key[SM4_GCM_KEY_SIZE];
	uint8_t* in = NULL;
	uint8_t* pt = NULL;
	size_t in_len = 0;

	if (parse_hex_exact(opts->key_hex, key, sizeof(key)) != 0)
	{
		fprintf(stderr, "[ERROR] invalid key hex (must be 32 hex chars)\n");
		return 1;
	}

	if (read_file_all(opts->input_path, &in, &in_len) != 0)
	{
		fprintf(stderr, "[ERROR] cannot read input file: %s\n", opts->input_path);
		return 1;
	}

	if (in_len < 4 + 1 + 1 + 1 + 1 + 4)
	{
		fprintf(stderr, "[ERROR] invalid TRS file (too short)\n");
		free(in);
		return 1;
	}

	size_t off = 0;
	if (memcmp(in + off, TRS_MAGIC, 4) != 0)
	{
		fprintf(stderr, "[ERROR] invalid TRS magic\n");
		free(in);
		return 1;
	}
	off += 4;

	uint8_t version = in[off++];
	uint8_t iv_len = in[off++];
	uint8_t tag_len = in[off++];
	(void)in[off++]; // reserved

	if (version != TRS_VERSION || iv_len != SM4_GCM_IV_SIZE || tag_len != SM4_GCM_TAG_SIZE)
	{
		fprintf(stderr, "[ERROR] unsupported TRS header\n");
		free(in);
		return 1;
	}

	uint32_t ct_len = read_u32_le(in + off);
	off += 4;

	if (off + iv_len + tag_len + ct_len != in_len)
	{
		fprintf(stderr, "[ERROR] invalid TRS length fields\n");
		free(in);
		return 1;
	}

	const uint8_t* iv = in + off;
	off += iv_len;
	const uint8_t* tag = in + off;
	off += tag_len;
	const uint8_t* ct = in + off;

	pt = (uint8_t*)malloc(ct_len ? ct_len : 1);
	if (!pt)
	{
		fprintf(stderr, "[ERROR] out of memory\n");
		free(in);
		return 1;
	}

	if (sm4_gcm_decrypt(
			key, iv, iv_len, (const uint8_t*)opts->aad, strlen(opts->aad), ct, ct_len, pt, tag)
		!= 0)
	{
		fprintf(stderr, "[ERROR] decrypt/auth verify failed\n");
		free(in);
		free(pt);
		return 1;
	}

	if (write_file_all(opts->output_path, pt, ct_len) != 0)
	{
		fprintf(stderr, "[ERROR] cannot write output file: %s\n", opts->output_path);
		free(in);
		free(pt);
		return 1;
	}

	free(in);
	free(pt);
	fprintf(stdout, "[OK] decrypted to %s\n", opts->output_path);
	return 0;
}

int main(int argc, char** argv)
{
	CliOptions opts;
	if (parse_args(argc, argv, &opts) != 0)
	{
		print_usage(argv[0]);
		return 1;
	}

	if (strcmp(opts.mode, "enc") == 0)
		return do_encrypt(&opts);
	return do_decrypt(&opts);
}
