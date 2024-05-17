extern "C" {
void vmult2(const unsigned int *Input_1,
		const unsigned int *Input_2,
		unsigned int *Output) {
#pragma HLS INTERFACE m_axi port=Input_1 bundle=aximm1
#pragma HLS INTERFACE m_axi port=Input_2 bundle=aximm2
#pragma HLS INTERFACE m_axi port=Output bundle=aximm1
	int Buffer_1[256][256];
	int Buffer_2[256][256];
	
	Init_loop_i: for (int i = 0; i < 256; i++)
	#pragma HLS pipeline off
		Init_loop_j: for (int j = 0; j < 256; j++) {
			#pragma HLS unroll factor=128
			#pragma HLS II=1
			Buffer_1[i][j] = Input_1[i * 256 + j];
			Buffer_2[i][j] = Input_2[i * 256 + j];
		}

	Main_loop_i: for (int i = 0; i < 256; i++)
		Main_loop_j: for (int j = 0; j < 256; j++) {
			#pragma HLS pipeline off
			int Result = 0;
			Main_loop_k: for (int k = 0; k < 256; k++) {
				#pragma HLS unroll factor=128
				#pragma HLS II=1
				Result += Buffer_1[i][k] * Buffer_2[k][j];
			}
			Output[i * 256 + j] = Result;
		}
	}
}