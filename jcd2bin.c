#include <stdio.h>
#include <string.h>

unsigned char jcd_header[0x800];
unsigned char buffer[0x100000];

int num_tracks, num_sessions;
int track_lba[256];
int track_msf[256][3];
int track_session[256];
int track_sector[256];
char str_buffer[512];
char temp_str[512];
int jcd_hack;


FILE *fp, *fp_cue, *fp_track;

void pause()
{
	while(getc(stdin) != '\n') {}
	return;
}

int msf2lba(int minutes, int seconds, int frames)
{
	return (((minutes * 60) + seconds) * 75) + frames;
}

void lba2msf(int lba, int* minutes, int* seconds, int* frames)
{
	*frames = lba % 75;
	lba /= 75;

	*seconds = lba % 60;
	lba /= 60;

	*minutes = lba;
}

void endian_swap(char *buffer, int size)
{
	int lcv;

	for( lcv = 0; lcv < size; lcv += 4 ) {
		int swap1 = buffer[lcv+0];
		int swap2 = buffer[lcv+1];
					
		buffer[lcv+0] = buffer[lcv+2];
		buffer[lcv+1] = buffer[lcv+3];
		buffer[lcv+2] = swap1;
		buffer[lcv+3] = swap2;
	}
}


void hackfix()
{
	int lcv, lcv2;

#if 0
	for( lcv = 0; lcv <= num_tracks; lcv++) {
		printf("%X\n", track_lba[lcv]);
	}
#endif

#if 0
	{
		static unsigned int elansar_lba[] = { 0, 0x46ea, 0x4944, 0x6839, 0x6d0a, 0x7a23, 0x824d, 0x837a };
		static unsigned int elansar_fix[] = { 0, 0x46e9, 0x4943, 0x6838, 0x6d09, 0x7a22, 0x824c, 0x837a };
		static unsigned int elansar_size = sizeof(elansar_lba) / 4;

		for( lcv = 0; (lcv < elansar_size) && (lcv <= num_tracks); ) {
			if( elansar_lba[lcv] != track_lba[lcv] ) break;
			if( (++lcv) < elansar_size ) continue;

			for( lcv2 = 1; lcv2 < elansar_size-1; lcv2++ ) {
				track_lba[lcv2]--;
				return;
			}
		}
	}
#endif


#if 1
	{
		static unsigned int ironsoldier2_lba[] = { 0, 0x7104, 0xb65e, 0xee41, 0x14442, 0x17be2, 0x1c252, 0x20b1a, 0x2539d, 0x2a11a, 0x2f150, 0x36c86, 0x37018, 0x3a77e, 0x3ff2c, 0x40381, 0x44079, 0x45634, 0x4608c, 0x48bf9, 0x48dba };
		static unsigned int ironsoldier2_size = sizeof(ironsoldier2_lba) / 4;

		for( lcv = 0; (lcv < ironsoldier2_size) && (lcv <= num_tracks); ) {
			if( ironsoldier2_lba[lcv] != track_lba[lcv] ) break;
			if( (++lcv) < ironsoldier2_size ) continue;

			for( lcv2 = 1; lcv2 < ironsoldier2_size; lcv2++ ) {
				track_lba[lcv2] += 149;
			}
			track_lba[1] += 1;
			track_lba[3] -= 1;
			track_lba[4] += 1;
			track_lba[11] -= 148;
			track_lba[12] += 1;
			track_lba[13] += 1;
			track_lba[14] += 2;
			track_lba[15] += 1;
			track_lba[16] += 1;
			track_lba[17] += 2;
			track_lba[18] += 1;
			track_lba[19] += 1;

			return;
		}
	}
#endif


#if 1
	{
		static unsigned int robinsonsrequiem_lba[] = { 0, 0x474c, 0x4a38, 0x4c8e, 0x4ee4, 0x513a, 0x5390, 0x55e6, 0x583c, 0x5a92, 0x5ce8, 0x5f3e, 0x6194, 0x63ea, 0x6640, 0x6896, 0x6aec, 0xc59d, 0x1473f, 0x1616c, 0x1776f, 0x1792e };
		static unsigned int robinsonsrequiem_size = sizeof(robinsonsrequiem_lba) / 4;

		for( lcv = 0; (lcv < robinsonsrequiem_size) && (lcv <= num_tracks); ) {
			if( robinsonsrequiem_lba[lcv] != track_lba[lcv] ) break;
			if( (++lcv) < robinsonsrequiem_size ) continue;

			track_lba[19] += 1;
			return;
		}
	}
#endif


#if 1
	{
		static unsigned int battlemorph_lba[] = { 0, 0x3ca4, 0x3ef9, 0x124b0, 0x17b35, 0x204a9, 0x28deb, 0x31773, 0x39fa2, 0x42916, 0x42ad5 };
		static unsigned int battlemorph_size = sizeof(battlemorph_lba) / 4;

		for( lcv = 0; (lcv < battlemorph_size) && (lcv <= num_tracks); ) {
			if( battlemorph_lba[lcv] != track_lba[lcv] ) break;
			if( (++lcv) < battlemorph_size ) continue;

			for( lcv2 = 2; lcv2 < battlemorph_size; lcv2++ ) {
				track_lba[lcv2] += 150;
			}
			return;
		}
	}
#endif
}


void main(int argc, char **argv)
{
	int lcv;
	int prev_session;
	int disc_type;
	int total_size = 0;


	if( argc < 2 ) {
		printf("Windows tip: Drag-and-drop jcd file onto exe or this console window\n\n");
		printf("Enter file path: ");
		fgets(str_buffer, 512-1, stdin);
		str_buffer[strlen(str_buffer)-1] = 0;

		if( str_buffer[0] == '\"' ) {
			str_buffer[strlen(str_buffer)-1] = 0;
			strcpy(str_buffer, str_buffer+1);
		}
	}
	else {
		printf("%s\n", argv[1]);
		strcpy(str_buffer, argv[1]);
	}

	fp = fopen(str_buffer, "rb");
	if( !fp ) {
		printf("Error: can't read %s\n", str_buffer);
		pause();
		return;
	}


	fread(jcd_header, 1, 0x800, fp);

	if( strncmp(jcd_header, "JCD\x00\x00\x00\x01", 7) != 0 ) {
		printf("Bad JCD header\n");
		pause();
		return;
	}


	num_tracks = jcd_header[7];
	num_sessions = jcd_header[8];

	track_lba[num_tracks] = msf2lba(jcd_header[9], jcd_header[10], jcd_header[11]);
	track_session[num_tracks] = num_sessions-1;

	for( lcv = 0; lcv < num_tracks; lcv++ ) {
		int ptr = 0x0C + lcv * 12;

		track_lba[lcv] = msf2lba(jcd_header[ptr + 1], jcd_header[ptr + 2], jcd_header[ptr + 3]);
		track_session[lcv] = jcd_header[ptr + 4];
		track_sector[lcv] = (jcd_header[ptr + 8] << 24) | (jcd_header[ptr + 9] << 16) | (jcd_header[ptr + 10] << 8) | (jcd_header[ptr + 11]);
	}

	fseek(fp, 0, SEEK_END);
	track_sector[num_tracks] = ftell(fp) / 512;


	hackfix();


	while( str_buffer[ strlen(str_buffer)-1 ] != '.' ) {
		str_buffer[ strlen(str_buffer)-1 ] = 0;
	}
	str_buffer[ strlen(str_buffer)-1 ] = 0;


	sprintf(temp_str, "%s.cue", str_buffer);
	fp_cue = fopen(temp_str, "w");

	if( !fp_cue ) {
		printf("Error: can't write %s\n", temp_str);
		pause();
		return;
	}



	prev_session = -1;
	disc_type = 0;

	for( lcv = 0; lcv < num_tracks; lcv++ ) {
		int ptr = 0x0C + lcv * 12;
		int track_id = jcd_header[ptr + 0];
		int session_id = jcd_header[ptr + 4];
		int jcd_size = msf2lba(jcd_header[ptr + 5], jcd_header[ptr + 6], jcd_header[ptr + 7]) * 2352;
		int track_size = (track_lba[lcv+1] - track_lba[lcv]) * 2352;
		int sector_size = (track_sector[lcv+1] - track_sector[lcv]) * 512;

		int lcv2;
		FILE *fp_track;
		int audio;


		printf("T%02d S%d | Start %X | Size %X | Sector %X | Size %X | Size %X\n", track_id, session_id,
			track_lba[lcv], track_size / 2352, track_sector[lcv], jcd_size / 2352, sector_size / 512);


		if( session_id == 0 ) {
			// mark cdi
			if( jcd_size != track_size ) {
				disc_type = 1;
			}

			if( disc_type == 1 ) {
				sector_size = jcd_size;

				if( track_id == 1 ) {
					track_size -= 150 * 2352;
				}
			}
		}

		if( disc_type == 1 ) {
			if( (session_id + 1) == track_session[lcv+1] ) {
				if( session_id == 0 ) {
					track_size -= 11250 * 2352;
				}

				else if( session_id == 1 ) {
					track_size -= 6750 * 2352;
				}

				else if( session_id >= 2 ) {
					track_size -= 2250 * 2352;
				}
			}
		}

		if( session_id != prev_session ) {
			fprintf(fp_cue, "REM SESSION %02d\n", session_id + 1);

			prev_session = session_id;

			if( disc_type == 1 ) {
				if( session_id >= 1 ) {
					track_size -= 150 * 2352;
				}
			}
		}


		if( disc_type == 1 ) {
			if( (session_id == 0) && (track_session[lcv] + 1) == track_session[lcv+1] ) {
				/* jcd hackfix = baldies, elansar */
				if( (track_size - sector_size) == 2352 ) {
					for( lcv2 = 1; lcv2 < num_tracks; lcv2++ ) {
						track_lba[lcv2] -= 1;
					}
					track_size -= 2352;
				}
			}
		}


		printf("T%02d S%d | Start %X | Size %X | Sector %X | Size %X | Size %X\n", track_id, session_id,
			track_lba[lcv], track_size / 2352, track_sector[lcv], jcd_size / 2352, sector_size / 512);


		if( num_tracks < 10 ) {
			sprintf(temp_str, "%s (Track %01d).bin", str_buffer, track_id);
		}
		else {
			sprintf(temp_str, "%s (Track %02d).bin", str_buffer, track_id);
		}

		fp_track = fopen(temp_str, "wb");
		if( !fp_track ) {
			printf("Error: can't write %s\n", temp_str);
			pause();
			return;
		}


		lcv2 = strlen(str_buffer);
		while( (str_buffer[lcv2] != '\\') && (str_buffer[lcv2] != '/') ) {
			lcv2--;

			if( lcv2 < 0 ) break;
		}


		if( num_tracks < 10 ) {
			fprintf(fp_cue, "FILE \"%s (Track %01d).bin\" BINARY\n", str_buffer+lcv2+1, track_id);
		}
		else {
			fprintf(fp_cue, "FILE \"%s (Track %02d).bin\" BINARY\n", str_buffer+lcv2+1, track_id);
		}


		audio = 0;
		if( session_id == 0 ) {
			audio = 1;

			if( track_id == 1 ) {
				char cd001[] = { 0x01, 'C', 'D' ,'0', '0', '1', 0x01 };

				fseek(fp, (track_sector[lcv] * 0x200) + 0x9208, SEEK_SET);
				fread(buffer, 1, 32, fp);
				endian_swap(buffer, 32);

				if( memcmp(buffer, cd001, sizeof(cd001) ) == 0 ) {
					printf("Data track detected\n");
					audio = 0;
				}
			}
		}

		if( audio == 1 ) {
			fprintf(fp_cue, "  TRACK %02d AUDIO\n", track_id);
		}
		else {
			fprintf(fp_cue, "  TRACK %02d MODE2/2352\n", track_id);
		}
		fprintf(fp_cue, "    INDEX 01 00:00:00\n");



		if( session_id >= 1 ) {
#if 1
			// note: "pregap" before ATRI marker
			// 0x02, 0x62, 0x13A, 0x2A2, ??
			int pregap = 2;

			memset(buffer, 0, pregap);
			fwrite(buffer, 1, pregap, fp_track);

			track_size -= pregap;
#endif
		}


		if( sector_size > track_size ) {
			sector_size = track_size;
		}


			
		fseek(fp, 0x200 * track_sector[lcv], SEEK_SET);

		//printf("[0] %X %X\n", sector_size, track_size);
		while( sector_size > 0 ) {
			int chunk;

			chunk = (sector_size < 0x100000) ? sector_size : 0x100000;
			fread(buffer, 1, chunk, fp);

			endian_swap(buffer, chunk);
			fwrite(buffer, 1, chunk, fp_track);

			sector_size -= chunk;
			track_size -= chunk;
		}


		//printf("[1] %X %X\n", sector_size, track_size);
		while( track_size > 0 ) {
			int chunk;

			chunk = (track_size < 0x100000) ? track_size : 0x100000;
			memset(buffer, 0, chunk);

			fwrite(buffer, 1, chunk, fp_track);

			track_size -= chunk;
		}


		fclose(fp_track);
	}
	fclose(fp_cue);
	fclose(fp);


	printf("End | %X\n", track_lba[lcv]);
	//getc(stdin);

	return;
}
