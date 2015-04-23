/* Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty. This file is offered as-is,
 * without any warranty.
 */

/*! @file process_frame.c
 * @brief Contains the actual algorithm and calculations.
 */

/* Definitions specific to this application. Also includes the Oscar main header file. */
#include "template.h"
#include <string.h>
#include <stdlib.h>

#define IMG_SIZE NUM_COLORS*(OSC_CAM_MAX_IMAGE_WIDTH/2)*(OSC_CAM_MAX_IMAGE_HEIGHT/2)

const int nc = OSC_CAM_MAX_IMAGE_WIDTH / 2;
const int nr = OSC_CAM_MAX_IMAGE_HEIGHT / 2;
const int Border = 6;
const int SizeBox = 5;

int avgDxy[3][IMG_SIZE];
int helpBuf[IMG_SIZE];
int MC[IMG_SIZE];
int absmax; // Prozentsatz von Maximum
int TextColor;

void CalcDeriv();
void AvgDeriv();
void Eck();

void ResetProcess() {
	//called when "reset" button is pressed
	if (TextColor == CYAN)
		TextColor = MAGENTA;
	else
		TextColor = CYAN;
}

void ProcessFrame() {
	uint32 t1, t2;
	char Text[] = "hallo world";
	//initialize counters
	if (data.ipc.state.nStepCounter == 1) {
		//use for initialization; only done in first step
		memset(data.u8TempImage[THRESHOLD], 0, IMG_SIZE);
		memset(avgDxy, 0, sizeof(avgDxy));
		TextColor = CYAN;
	} else {
		//example for time measurement
		t1 = OscSupCycGet();
		//example for copying sensor image to background image
		//memcpy(data.u8TempImage[BACKGROUND], data.u8TempImage[SENSORIMG],
		//IMG_SIZE);
		CalcDeriv();
		AvgDeriv(0);
		AvgDeriv(1);
		AvgDeriv(2);
		absmax=0;
		Eck();
		absmax = (absmax/100*data.ipc.state.nThreshold);

				//example for time measurement
				t2 = OscSupCycGet();

				//example for log output to console
				OscLog(INFO, "required = %d us\n",
						OscSupCycToMicroSecs(t2 - t1));

				//example for drawing output
				//draw line
				//DrawLine(10, 100, 200, 20, RED);
				//draw open rectangle
				//DrawBoundingBox(20, 10, 50, 40, false, GREEN);
				//draw filled rectangle
				//DrawBoundingBox(80, 100, 110, 120, true, BLUE);
				//DrawString(200, 200, strlen(Text), TINY, TextColor, Text);
			}
		}
		void CalcDeriv() {	// derivation = Ableitung
			int c, r;
			for (r = nc; r < nr * nc - nc; r += nc) {/* we skip the first and last line */
				for (c = 1; c < nc - 1; c++) {
					/* do pointer arithmetics with respect to center pixel location */
					unsigned char* p = &data.u8TempImage[SENSORIMG][r + c];
					/* implement Sobel filter */
					int dx = -(int) *(p - nc - 1) + (int) *(p - nc + 1)
							- 2 * (int) *(p - 1) + 2 * (int) *(p + 1)
							- (int) *(p + nc - 1) + (int) *(p + nc + 1);
					int dy = -(int) *(p - nc - 1l) - 2 * (int) *(p - nc)
							- (int) *(p - nc + 1) + (int) *(p + nc - 1)
							+ 2 * (int) *(p + nc) + (int) *(p + nc + 1);

					avgDxy[0][r + c] = dx * dx;
					avgDxy[1][r + c] = dy * dy;
					avgDxy[2][r + c] = dx * dy;

//data.u8TempImage[BACKGROUND][r+c] = (uint8) MIN(255, MAX(0, 128+dx));
					data.u8TempImage[BACKGROUND][r + c] = 0; //MAX(0, MIN(255, 128+ (dx >> 10))); >>2 Division durch 4
					data.u8TempImage[THRESHOLD][r + c] = MAX(0,
							MIN(255, 128+ (dx*dy >> 10)));
				}
			}
		}

		void AvgDeriv(int Index) {
//do average in x-direction
			int c, r;
			for (r = nc; r < nr * nc - nc; r += nc) {/* we skip first and last lines (empty) */
				for (c = Border + 1; c < nc - (Border + 1); c++) {/* +1 because we have one empty border column */
					/* do pointer arithmetics with respect to center pixel location */
					int* p = &avgDxy[Index][r + c];
					int sx = (*(p - 6) + *(p + 6)) * 1
							+ (*(p - 5) + *(p + 5)) * 4
							+ (*(p - 4) + *(p + 4)) * 11
							+ (*(p - 3) + *(p + 3)) * 27
							+ (*(p - 2) + *(p + 2)) * 50
							+ (*(p - 1) + *(p + 1)) * 72 + (*p) * 82;
//now averaged
					helpBuf[r + c] = (sx >> 8);
				}
			}
//do average in y-direction
			for (r = nc * (Border + 1); r < nr * nc - nc * (Border + 1); r +=
					nc) {
				for (c = Border + 1; c < nc - (Border + 1); c++) {
					int* p = &helpBuf[r + c];
					int sy = (*(p - 6 * nc) + *(p + 6 * nc))
							+ ((*(p - 5 * nc) + *(p + 5 * nc)) << 2)
							+ (*(p - 4 * nc) + *(p + 4 * nc)) * 11
							+ (*(p - 3 * nc) + *(p + 3 * nc)) * 27
							+ (*(p - 2 * nc) + *(p + 2 * nc)) * 50
							+ (*(p - 1 * nc) + *(p + 1 * nc)) * 72 + (*p) * 82;

					avgDxy[Index][r + c] = (sy >> 8);

					data.u8TempImage[BACKGROUND][r + c] = MAX(0,
							h MIN(255, (avgDxy[Index][r+c]>>11)));

				}
			}
		}
		void LocalMaximum() {
	int c, r;
	for (r = 7 * nc; r < nr * nc - 7 * nc; r += nc) {/* we skip the first six and last six line */
		for (c = 7; c < nc - 7; c++) {
			/* do pointer arithmetics with respect to center pixel location */
			int* p = &MC[c + r];
			/* implement max filter */
			int localMax = 0;
			for (int i = -6; i < 7; i++) {
				for (int j = -6; j < 7; j++) {
					if (localMax <= *(p + nc * i + j)) {
						localMax = *(p + i * nc + j);
					}
				}
			}

			if (localMax == *p && *p > absmax) { // Maximas mit grüner Box markieren
				DrawBoundingBox(c - SizeBox, r / nc + SizeBox, c + SizeBox,
						r / nc - SizeBox, false, GREEN);
			}
		}
	}
}
		void Eck()
		{for (int r = nc; r < nr * nc - nc; r += nc)
		{/* we skip the first six and last six line. Boarders */
				for (int c = 7; c < nc - 7; c++) {
						int i = r + c;
						MC[i] = ((avgDxy[0][i] >> 6) * (avgDxy[1][i] >> 6)
						- (avgDxy[2][i] >> 6) * (avgDxy[2][i] >> 6))
						- ((5*((avgDxy[0][i] >> 6)
										+ (avgDxy[1][i] >> 6))
								* ((avgDxy[0][i] >> 6)
										+ (avgDxy[1][i] >> 6))) >> 7);
						absmax = MAX(MC[i], absmax); //Ermittlung Maximum über gesamtes Bild
					}
				}
		}

