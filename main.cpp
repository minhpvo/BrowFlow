#ifdef _WINDOWS
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif
#include <omp.h>
#include "project.h"
#include "Image.h"
#include "OpticalFlow.h"

void makeDir(char *Fname)
{
#ifdef _WINDOWS
	mkdir(Fname);
#else
	mkdir(Fname, 0755);
#endif
	return;
}
int IsFileExist(char *Fname)
{
	std::ifstream test(Fname);
	if (test.is_open())
	{
		test.close();
		return 1;
	}
	else
		return 0;
}
void PrintFlowData(char *fnX, char *fnY, DImage &vx, DImage &vy)
{
	FILE *fp1 = fopen(fnX, "w+");
	FILE *fp2 = fopen(fnY, "w+");

	int width = vx.width(), height = vx.height();
	for (int jj = 0; jj < height; jj++)
	{
		for (int ii = 0; ii < width; ii++)
		{
			fprintf(fp1, "%.3f ", vx.pData[ii + jj*width]);
			fprintf(fp2, "%.3f ", vy.pData[ii + jj*width]);
		}
		fprintf(fp1, "\n");
		fprintf(fp2, "\n");
	}
	fclose(fp1);
	fclose(fp2);
	return;
}
bool WriteFlowDataBinary(char *fnX, char *fnY, DImage &vx, DImage &vy, int width, int height)
{
	float u, v;

	ofstream fout1, fout2;
	fout1.open(fnX, ios::binary);
	if (!fout1.is_open())
	{
		cout << "Cannot write: " << fnX << endl;
		return false;
	}
	fout2.open(fnY, ios::binary);
	if (!fout2.is_open())
	{
		cout << "Cannot write: " << fnY << endl;
		return false;
	}

	for (int j = 0; j < height; ++j)
	{
		for (int i = 0; i < width; ++i)
		{
			u = (float)vx.pData[i + j*width];
			v = (float)(vy.pData[i + j*width]);

			fout1.write(reinterpret_cast<char *>(&u), sizeof(float));
			fout2.write(reinterpret_cast<char *>(&v), sizeof(float));
		}
	}
	fout1.close();
	fout2.close();

	return true;
}
bool ReadFlowDataBinary(char *fnX, char *fnY, float *fx, float *fy, int width, int height)
{
	float u, v;

	ifstream fin1, fin2;
	fin1.open(fnX, ios::binary);
	if (!fin1.is_open())
	{
		cout << "Cannot load: " << fnX << endl;
		return false;
	}
	fin2.open(fnY, ios::binary);
	if (!fin2.is_open())
	{
		cout << "Cannot load: " << fnY << endl;
		return false;
	}

	for (int j = 0; j < height; ++j)
	{
		for (int i = 0; i < width; ++i)
		{
			fin1.read(reinterpret_cast<char *>(&u), sizeof(float));
			fin2.read(reinterpret_cast<char *>(&v), sizeof(float));

			fx[i + j*width] = u;
			fy[i + j*width] = v;
		}
	}
	fin1.close();
	fin2.close();

	return true;
}
void VariationalFlowWithInit_Demo()
{
	// get the parameters
	double alpha = 0.015;
	double ratio = 0.75;
	int minWidth = 20;
	int nOuterFPIterations = 7;
	int nInnerFPIterations = 1;
	int nSORIterations = 30;

	DImage Im1, Im2, Im3, Im4, vx12, vy12, vx23, vy23, vx13, vy13, vx34, vy34, vx14, vy14, warpI2;
	//1->2
	Im1.imread("C:\\temp\\C1\\21-33-17;19.bmp");
	Im2.imread("C:\\temp\\C1\\21-33-17;20.bmp");
	Im3.imread("C:\\temp\\C1\\21-33-17;21.bmp");
	Im4.imread("C:\\temp\\C1\\21-33-17;22.bmp");

	OpticalFlow::Coarse2FineFlow(vx12, vy12, warpI2, Im1, Im2, alpha, ratio, minWidth, nOuterFPIterations, nInnerFPIterations, nSORIterations);
	OpticalFlow::Coarse2FineFlow(vx23, vy23, warpI2, Im2, Im3, alpha, ratio, minWidth, nOuterFPIterations, nInnerFPIterations, nSORIterations);

	vx13.copyData(vx12); vx13.Add(vx23);
	vy13.copyData(vy12); vy13.Add(vy23);
	OpticalFlow::FineFlowWInit(vx13, vy13, warpI2, Im1, Im3, alpha, ratio, minWidth, nOuterFPIterations, nInnerFPIterations, nSORIterations);

	OpticalFlow::Coarse2FineFlow(vx34, vy34, warpI2, Im3, Im4, alpha, ratio, minWidth, nOuterFPIterations, nInnerFPIterations, nSORIterations);
	vx14.copyData(vx13); vx14.Add(vx34);
	vy14.copyData(vy13); vy14.Add(vy34);

	OpticalFlow::FineFlowWInit(vx14, vy14, warpI2, Im1, Im4, alpha, ratio, minWidth, nOuterFPIterations, nInnerFPIterations, nSORIterations);
	warpI2.imwrite("C:\\temp\\wI12.bmp");
	PrintFlowData("C:\\temp\\F1_12x.dat", "C:\\temp\\F1_12y.dat", vx14, vy14);

	return;
}
int main(int argc, char** argv)
{
	if (argc == 1)
	{
		printf("Usage: BroxOF.exe Path camID startF stopF (alpha) (ratio)\n");
		printf("Set stopF smaller than startF to compute backward flow\n");
		return 1;
	}

	char *Path = argv[1];
	char Fname1[200], Fname2[200];

	int camID = atoi(argv[2]), startF = atoi(argv[3]), stopF = atoi(argv[4]);

	double alpha = 0.015, ratio = 0.85;
	int minWidth = 40;
	int nOuterFPIterations = 7, nInnerFPIterations = 2, nSORIterations = 30;

	if (argc == 6)
		alpha = atof(argv[5]);
	if (argc == 7)
		alpha = atof(argv[5]), ratio = atof(argv[6]);

	DImage Im1, Im2;
	DImage vx, vy, warpI2;

	sprintf(Fname1, "%s/%d/Flow", Path, camID); makeDir(Fname1);
	sprintf(Fname1, "%s/%d/Warped", Path, camID); makeDir(Fname1);

	double start = omp_get_wtime();
	if (startF < stopF)
	{
		for (int fid = startF; fid < stopF; fid++)
		{
			printf("%d ..", fid);

			sprintf(Fname1, "%s/%d/Flow/X_%d_%d.dat", Path, camID, fid, fid + 1);
			sprintf(Fname2, "%s/%d/Flow/Y_%d_%d.dat", Path, camID, fid, fid + 1);
			if (IsFileExist(Fname1) == 1 && IsFileExist(Fname2) == 1)
				continue;

			sprintf(Fname1, "%s/%d/%d.png", Path, camID, fid);
			if (IsFileExist(Fname1) == 0)
				sprintf(Fname1, "%s/%d/%d.jpg", Path, camID, fid);
			Im1.imread(Fname1);
			if (!Im1.imread(Fname1))
			{
				printf("Cannot load %s\n", Fname1);
				return 1;
			}
			sprintf(Fname2, "%s/%d/%d.png", Path, camID, fid + 1);
			if (IsFileExist(Fname2) == 0)
				sprintf(Fname2, "%s/%d/%d.jpg", Path, camID, fid + 1);
			Im2.imread(Fname2);
			if (!Im2.imread(Fname2))
			{
				printf("Cannot load %s\n", Fname2);
				return 1;
			}

			OpticalFlow::Coarse2FineFlow(vx, vy, warpI2, Im1, Im2, alpha, ratio, minWidth, nOuterFPIterations, nInnerFPIterations, nSORIterations);

			sprintf(Fname1, "%s/%d/Flow/X_%d_%d.dat", Path, camID, fid, fid + 1);
			sprintf(Fname2, "%s/%d/Flow/Y_%d_%d.dat", Path, camID, fid, fid + 1);
			WriteFlowDataBinary(Fname1, Fname2, vx, vy, vx.width(), vx.height());

			sprintf(Fname1, "%s/%d/Warped/F_%d.jpg", Path, camID, fid);
			warpI2.imwrite(Fname1);
		}
	}
	else
	{
		for (int fid = startF; fid > stopF; fid--)
		{
			printf("%d ..", fid);
			sprintf(Fname1, "%s/%d/Flow/X_%d_%d.dat", Path, camID, fid, fid - 1);
			sprintf(Fname2, "%s/%d/Flow/Y_%d_%d.dat", Path, camID, fid, fid - 1);
			if (IsFileExist(Fname1) == 1 && IsFileExist(Fname2) == 1)
				continue;

			sprintf(Fname1, "%s/%d/%d.png", Path, camID, fid);
			if (IsFileExist(Fname1) == 0)
				sprintf(Fname1, "%s/%d%d.jpg", Path, camID, fid);
			Im1.imread(Fname1);
			if (!Im1.imread(Fname1))
			{
				printf("Cannot load %s\n", Fname1);
				return 1;
			}
			sprintf(Fname2, "%s/%d/%d.png", Path, camID, fid - 1);
			if (IsFileExist(Fname2) == 0)
				sprintf(Fname2, "%s/%d%d.jpg", Path, camID, fid - 1);
			Im2.imread(Fname2);
			if (!Im2.imread(Fname2))
			{
				printf("Cannot load %s\n", Fname2);
				return 1;
			}

			OpticalFlow::Coarse2FineFlow(vx, vy, warpI2, Im1, Im2, alpha, ratio, minWidth, nOuterFPIterations, nInnerFPIterations, nSORIterations);

			sprintf(Fname1, "%s/%d/Flow/X_%d_%d.dat", Path, camID, fid, fid - 1);
			sprintf(Fname2, "%s/%d/Flow/Y_%d_%d.dat", Path, camID, fid, fid - 1);
			WriteFlowDataBinary(Fname1, Fname2, vx, vy, vx.width(), vx.height());

			sprintf(Fname1, "%s/%d/Warped/B_%d.jpg", Path, camID, fid);
			warpI2.imwrite(Fname1);

		}
	}
	printf("\nDone. %.2fs", omp_get_wtime() - start);

	return 0;
}