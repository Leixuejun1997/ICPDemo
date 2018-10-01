/***************************************************************************************************************************************/
/*ͷ�ļ�����;*/
#include <iostream>
#include <time.h>
#include <string>
#include <vector>
#include <math.h>
#include <map>
#include <pcl/io/io.h>
#include <pcl/io/pcd_io.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/common/centroid.h>
#include <pcl/registration/icp.h>
#include<pcl/common/transforms.h>

using namespace std;

#define max(a,b)((a>b)?b:a)
#define min(a,b)((a<b)?b:a)

/***************************************************************************************************************************************/
/*����;*/
struct My_Polygon
{
	pcl::PointCloud<pcl::PointXYZ>::Ptr border;
	Eigen::Vector3f normal;
};

/***************************************************************************************************************************************/
/*ȫ�ֺ����ͱ���;*/
pcl::PointCloud<pcl::PointXYZ>::Ptr source_cloud_border(new pcl::PointCloud<pcl::PointXYZ>);        //Դ����ƽ��߽�;
pcl::PointCloud<pcl::PointXYZ>::Ptr target_cloud_border(new pcl::PointCloud<pcl::PointXYZ>);        //Ŀ�����ƽ��߽�;

pcl::PointCloud<pcl::PointXYZ>::Ptr source_cloud(new pcl::PointCloud<pcl::PointXYZ>);				//Դ����;
pcl::PointCloud<pcl::PointXYZ>::Ptr target_cloud(new pcl::PointCloud<pcl::PointXYZ>);				//Ŀ�����;
pcl::PointCloud<pcl::PointXYZ>::Ptr source_cloud_backup(new pcl::PointCloud<pcl::PointXYZ>);
pcl::PointCloud<pcl::PointXYZ>::Ptr reg_result(new pcl::PointCloud<pcl::PointXYZ>);

pcl::visualization::PCLVisualizer viewer;

Eigen::Matrix4f transformation_matrix; //�任����;


My_Polygon S_polyAfterN;																			//��һ��֮��Ķ����;
My_Polygon T_polyAfterN;
vector<My_Polygon> PolyAfterN(4);

pcl::PointCloud<pcl::PointXYZ>::Ptr S_center(new pcl::PointCloud<pcl::PointXYZ>);				    //��Ӧ����ε�����;
pcl::PointCloud<pcl::PointXYZ>::Ptr T_center(new pcl::PointCloud<pcl::PointXYZ>);

My_Polygon P_poly;																					//�������ֶ����;
vector<My_Polygon> source_polygon;																	//Դ����μ���;
vector<My_Polygon> target_polygon;																	//Ŀ�����μ���;


clock_t start;																						//ʱ��;
clock_t finish;

const float sigma = 0.85;																			//ƥ�����;
const int vertex_diff = 20;																			//�������;   

int source_index = 0;																				//��Ȼƥ��ʱ������;
int target_index = -1;
int p_index = 0;

std::vector<int> S_final_index;																		//��Ӧ���������;
std::vector<int> T_final_index;

bool IsMatch = false;																				//�ж�ƥ���Ƿ�ɹ�;

const int CMD_SIZE = 64;
char cmd[CMD_SIZE];
char state[CMD_SIZE];

DWORD WINAPI cmdFunc(LPVOID);
DWORD dwThread;
HANDLE dispaly_thread;

/***************************************************************************************************************************************/
/*ȫ�ֺ���;*/
void processStateMsg();                                                                             //��Ϣ������;
void loadpolygon();                                                                                 //���ض����;
void FindSimilarPoly();																				//Ѱ�����ƶ����;
void registration();																				//��׼����;
void Match();																						//��������ι�������ƥ��;
void normalize();																					//��һ������;
float getPolyArea(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud);										//���������;
bool PublicArea();																					//��ȡ��������ι�������;
bool GetCrossPoint(int s_first, int s_next, int t_first, int t_next, float &x, float &y);			//�������߶εĽ���;
bool IsRectCross(int s_first, int s_next, int t_first, int t_next);									//�ų�ʵ��;
bool IsLineSegmentCross(int s_first, int s_next, int t_first, int t_next);							//�����ж�;
bool IsPointInPolygon(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud, const pcl::PointXYZ &pt);			//�жϵ��ǲ����ڶ�����ڲ�; 
void ClockwiseSortPoints();																			//�㼯����;
void registration_cloud();																			//��ʼ������׼;
void icp();
void keyboardEventOccurred(const pcl::visualization::KeyboardEvent &event, void* viewer_void);

/***************************************************************************************************************************************/
/*main����;*/

int main()
{
	srand(time(0));
	dispaly_thread = ::CreateThread(NULL, 0, cmdFunc, (LPVOID)cmdFunc, 0, &dwThread);
	viewer.addCoordinateSystem(1.0f);
	viewer.setBackgroundColor(0, 0, 0);
	viewer.setWindowName("Point Cloud Register");
	viewer.registerKeyboardCallback(keyboardEventOccurred, (void*)&viewer);
	while (!viewer.wasStopped())
	{
		viewer.spinOnce(500);
		processStateMsg();
	}
	return 0;
}

/***************************************************************************************************************************************/
/*�����в���;*/
DWORD WINAPI cmdFunc(LPVOID)																		//������;
{
	while (true)
	{
		memset(cmd, 0, sizeof(char)*CMD_SIZE);
		cout << ">";

		int i = 0;
		do
		{
			char c;
			c = getchar();
			if (c == '\n')
				break;
			cmd[i] = c;
			++i;
		} while (1);
		if (strcmp(cmd, "load polygon") == 0)
		{
			start = std::clock();
			loadpolygon();
			finish = std::clock();
			cout << "load polygon cost " << finish - start << " ms." << endl;
			continue;
		}
		if (strcmp(cmd, "find") == 0)
		{
			FindSimilarPoly();
			continue;
		}
		if (strcmp(cmd, "next") == 0)
		{
			memset(state, 0, CMD_SIZE);
			strcpy(state, cmd);
			continue;
		}
		if (strcmp(cmd, "registration") == 0)
		{
			memset(state, 0, CMD_SIZE);
			strcpy(state, cmd);
			continue;
		}
		if (strcmp(cmd, "register cloud") == 0)
		{
			memset(state, 0, CMD_SIZE);
			strcpy(state, cmd);
			continue;
		}

		if (strcmp(cmd, "do") == 0)																	//�Զ�����;
		{
			memset(state, 0, CMD_SIZE);
			strcpy(state, cmd);
			continue;
		}
		if (strcmp(cmd, "reboot") == 0)
		{
			memset(state, 0, CMD_SIZE);
			strcpy(state, cmd);
			continue;
		}
		if (strcmp(cmd, "icp") == 0)
		{
			memset(state, 0, CMD_SIZE);
			strcpy(state, cmd);
			continue;
		}

	}
}
void processStateMsg()
{
	if (strcmp(state, "reboot") == 0)
	{
		viewer.removeAllPointClouds();
		int source_index = 0;
		int target_index = -1;
		bool IsMatch = false;
		source_cloud_border->clear();
		target_cloud_border->clear();
		source_polygon.clear();
		target_polygon.clear();
		cout << "reboot success....." << endl;
		memset(state, 0, CMD_SIZE);
	}
	if (strcmp(state, "next") == 0)
	{
		source_index++;
		cout << "match continue..." << endl;
		IsMatch = false;
		FindSimilarPoly();
		memset(state, 0, CMD_SIZE);
	}
	if (strcmp(state, "do") == 0)
	{
		start = std::clock();
		cout << "auto run......." << endl;
		loadpolygon();
		while (source_index < source_polygon.size())
		{
			FindSimilarPoly();
			cout << "match continue..." << endl;
			source_index++;
			IsMatch = false;
		}
		registration();
		registration_cloud();
		icp();
		finish = std::clock();
		cout << "finally cost " << finish - start << " ms." << endl;
		memset(state, 0, CMD_SIZE);
	}
	if (strcmp(state, "registration") == 0)
	{
		registration();
		memset(state, 0, CMD_SIZE);
	}
	if (strcmp(state, "register cloud") == 0)
	{
		start = std::clock();
		registration_cloud();
		finish = std::clock();
		cout << "finally cost " << finish - start << " ms." << endl;
		memset(state, 0, CMD_SIZE);
	}
	if (strcmp(state, "icp") == 0)
	{
		icp();
		memset(state, 0, CMD_SIZE);
	}
}
void keyboardEventOccurred(const pcl::visualization::KeyboardEvent &event, void* viewer_void)
{
	if (event.getKeySym() == "F1" && event.keyDown())
	{
		viewer.removeAllPointClouds();
		viewer.addPointCloud(source_cloud_backup, "source");
		viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 1.0, 0, 0, "source");
		viewer.addPointCloud(target_cloud, "target");
		viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 0, 1.0, 0, "target");
	}
	if (event.getKeySym() == "F2" && event.keyDown())
	{

		viewer.removeAllPointClouds();
		viewer.addPointCloud(source_cloud, "source");
		viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 1.0, 0, 0, "source");
		viewer.addPointCloud(target_cloud, "target");
		viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 0, 1.0, 0, "target");
	}
	if (event.getKeySym() == "F3" && event.keyDown())
	{
		viewer.removeAllPointClouds();
		viewer.addPointCloud(reg_result, "cloud");
		viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 1.0, 1.0, 0, "cloud");
	}

}
/***************************************************************************************************************************************/
/*����Ŀ�ꡢԴ�����Լ����ƽ������;*/
void loadpolygon()																					//���ص���;
{
	cout << "please wait, the polygon is loading..." << endl;										//���ض���ο�ʼ�����ȴ�Դ���ƿ�ʼ;

	source_cloud_border->clear();                                                                   //��ʼ��;
	target_cloud_border->clear();

	int num;
	int sum = 0;
	int i = 0;

	pcl::PointXYZ p;
	My_Polygon polygon;

	vector<int> source;																				//������ÿ������ε�ĸ���;
	vector<int> target;
	vector<int>::iterator it;

	pcl::io::loadPCDFile<pcl::PointXYZ>("dataForPlane/source_cloud.pcd", *source_cloud);
	pcl::io::loadPCDFile<pcl::PointXYZ>("dataForPlane/target_cloud.pcd", *target_cloud);
	*source_cloud_backup = *source_cloud;

	if (pcl::io::loadPCDFile<pcl::PointXYZ>("dataForPlane/source.pcd", *source_cloud_border) == -1)              //��Դ�����;
	{
		PCL_ERROR("Couldn't read file source.pcd \n");
		return;
	}
	ifstream file("dataForPlane/source.txt");
	while (1)
	{
		file >> num;
		sum += num;
		source.push_back(sum);
		if (sum >= source_cloud_border->size())
		{
			break;
		}
	}
	it = source.begin();
	while (it != source.end())
	{
		pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
		for (; i < *it; i++)
		{
			p = source_cloud_border->points[i];
			cloud->points.push_back(p);
		}
		cloud->width = cloud->points.size();
		cloud->height = 1;
		cloud->is_dense = false;
		cloud->points.resize(cloud->width * cloud->height);

		polygon.border = cloud;
		source_polygon.push_back(polygon);

		it++;
	}
	cout << "load source polygon success, total has " << source_polygon.size() << "polygon" << endl;

	if (pcl::io::loadPCDFile<pcl::PointXYZ>("dataForPlane/target.pcd", *target_cloud_border) == -1)
	{
		PCL_ERROR("Couldn't read file target.pcd \n");
		return;
	}
	ifstream file1("dataForPlane/target.txt");

	sum = 0;
	i = 0;

	while (1)
	{
		file1 >> num;
		sum += num;
		target.push_back(sum);
		if (sum >= target_cloud_border->size())
		{
			break;
		}
	}
	it = target.begin();
	while (it != target.end())
	{
		pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
		for (; i < *it; i++)
		{
			p = target_cloud_border->points[i];
			cloud->points.push_back(p);
		}
		cloud->width = cloud->points.size();
		cloud->height = 1;
		cloud->is_dense = false;
		cloud->points.resize(cloud->width * cloud->height);

		polygon.border = cloud;
		target_polygon.push_back(polygon);

		it++;
	}
	cout << "load target polygon success, total has " << target_polygon.size() << "polygon" << endl;
}

/***************************************************************************************************************************************/
/*����γ�ƥ��;*/
void FindSimilarPoly()																			    //������Դ���������ѡȡһ������Σ���Ŀ���������ñ߽��������г�ʼƥ��;
{
	int source_num;																					//ȡ��Ӧ����ε���Ŀ; 
	int target_num;

	while (source_index < source_polygon.size())
	{
		source_num = source_polygon[source_index].border->size();

		for (int i = 0; i < target_polygon.size(); ++i)
		{
			target_num = target_polygon[i].border->size();

			if (abs(target_num - source_num) <= vertex_diff)										//�����������εĶ����ֵ��һ����ֵ�ڣ�����Ϊ���Խ���ƥ��;
			{
				target_index = i;
				cout << "there are two polygon start to math " << endl;
				cout << "source_index = " << source_index << ", total has " << source_num << " vertexes." << endl;
				cout << "target_index = " << target_index << ", total has " << target_num << " vertexes." << endl;
				normalize();

				if (IsMatch == true)                                                                //ƥ��ɹ�����forѭ��;
				{
					viewer.removeAllPointClouds();
					viewer.addPointCloud<pcl::PointXYZ>(S_polyAfterN.border, "s");
					viewer.addPointCloud<pcl::PointXYZ>(PolyAfterN[p_index].border, "t");
					viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 255, 0, 0, "s");
					viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 0, 255, 0, "t");
					break;
				}
			}
		}
		if (target_index != -1 && IsMatch == true)												   //��ʾƥ��ɹ�������while;
		{
			break;
		}
		else																					   //else��ʾ������Դ�������Ŀ��������û���ҵ�ƥ��;
		{
			source_index++;
		}
	}
	if (source_index >= source_polygon.size())
	{
		cout << "find finish!...." << endl;
	}
}

/***************************************************************************************************************************************/
/*����ι�һ��;*/
void normalize()
{
	pcl::PointCloud<pcl::PointXYZ>::Ptr S_cloudAfterN(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr T_cloudAfterN(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr p_cloud0(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr p_cloud1(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr p_cloud2(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr p_cloud3(new pcl::PointCloud<pcl::PointXYZ>);

	pcl::PointXYZ p, p1, p2, p3;

	*S_cloudAfterN = *source_polygon[source_index].border;
	*T_cloudAfterN = *target_polygon[target_index].border;

	S_polyAfterN.border = S_cloudAfterN;
	T_polyAfterN.border = T_cloudAfterN;

	PolyAfterN[0].border = p_cloud0;
	PolyAfterN[1].border = p_cloud1;
	PolyAfterN[2].border = p_cloud2;
	PolyAfterN[3].border = p_cloud3;

	Eigen::Matrix3f covariance_matrix_source, covariance_matrix_target;                           //Э�������;
	Eigen::Vector4f xyz_centroid_source, xyz_centroid_target;									  //���ĵ�;
	Eigen::Vector3f eigen_values_source, eigen_values_target;									  //����ֵ����������;
	Eigen::Matrix3f eigen_vectors_source, eigen_vectors_target;

	pcl::computeMeanAndCovarianceMatrix(*S_cloudAfterN, covariance_matrix_source, xyz_centroid_source);
	pcl::eigen33(covariance_matrix_source, eigen_vectors_source, eigen_values_source);			  //����Э�����������ֵ����������;

	pcl::computeMeanAndCovarianceMatrix(*T_cloudAfterN, covariance_matrix_target, xyz_centroid_target);
	pcl::eigen33(covariance_matrix_target, eigen_vectors_target, eigen_values_target);			  //����Э�����������ֵ����������;

	for (int i = 0; i < source_polygon[source_index].border->points.size(); ++i)
	{
		S_cloudAfterN->points[i].x = S_cloudAfterN->points[i].x - xyz_centroid_source[0];		  //���Ƚ�����ƽ�Ƶ���������;
		S_cloudAfterN->points[i].y = S_cloudAfterN->points[i].y - xyz_centroid_source[1];
		S_cloudAfterN->points[i].z = S_cloudAfterN->points[i].z - xyz_centroid_source[2];

		p.x = S_cloudAfterN->points[i].x * eigen_vectors_source.coeff(6)						  //��������任;
			+ S_cloudAfterN->points[i].y * eigen_vectors_source.coeff(7)
			+ S_cloudAfterN->points[i].z * eigen_vectors_source.coeff(8);

		p.y = S_cloudAfterN->points[i].x * eigen_vectors_source.coeff(3)
			+ S_cloudAfterN->points[i].y * eigen_vectors_source.coeff(4)
			+ S_cloudAfterN->points[i].z * eigen_vectors_source.coeff(5);

		p.z = 0;

		S_cloudAfterN->points[i] = p;
	}

	for (int i = 0; i < target_polygon[target_index].border->points.size(); ++i)
	{
		T_cloudAfterN->points[i].x = T_cloudAfterN->points[i].x - xyz_centroid_target[0];
		T_cloudAfterN->points[i].y = T_cloudAfterN->points[i].y - xyz_centroid_target[1];
		T_cloudAfterN->points[i].z = T_cloudAfterN->points[i].z - xyz_centroid_target[2];

		p.x = T_cloudAfterN->points[i].x * eigen_vectors_target.coeff(6)
			+ T_cloudAfterN->points[i].y * eigen_vectors_target.coeff(7)
			+ T_cloudAfterN->points[i].z * eigen_vectors_target.coeff(8);

		p.y = T_cloudAfterN->points[i].x * eigen_vectors_target.coeff(3)
			+ T_cloudAfterN->points[i].y * eigen_vectors_target.coeff(4)
			+ T_cloudAfterN->points[i].z * eigen_vectors_target.coeff(5);

		p.z = 0;
		T_cloudAfterN->points[i] = p;
	}
	*PolyAfterN[0].border = *T_cloudAfterN;
	for (int i = 0; i < PolyAfterN[0].border->size(); ++i)
	{
		p = PolyAfterN[0].border->points[i];
		p1.x = p.x; p1.y = -1 * p.y; p1.z = p.z;
		p2.x = -1 * p.x; p2.y = p.y; p2.z = p.z;
		p3.x = -1 * p.x; p3.y = -1 * p.y; p3.z = p.z;
		PolyAfterN[1].border->points.push_back(p1);
		PolyAfterN[2].border->points.push_back(p2);
		PolyAfterN[3].border->points.push_back(p3);
	}
	cout << "coordinate transformation success!, start to match.... " << endl;
	Match();																					  //��ת�������������ν��й�������ƥ��;
}

/***************************************************************************************************************************************/
/*��������ƥ��;*/
bool IsRectCross(int s_first, int s_next, int t_first, int t_next)								  //�ų�ʵ��;
{
	pcl::PointXYZ p1, p2, q1, q2;
	p1 = S_polyAfterN.border->points[s_first];
	p2 = S_polyAfterN.border->points[s_next];
	q1 = T_polyAfterN.border->points[t_first];
	q2 = T_polyAfterN.border->points[t_next];

	bool ret = min(p1.x, p2.x) <= max(q1.x, q2.x) &&
		min(q1.x, q2.x) <= max(p1.x, p2.x) &&
		min(p1.y, p2.y) <= max(q1.y, q2.y) &&
		min(q1.y, q2.y) <= max(p1.y, p2.y);

	return ret;
}

bool IsLineSegmentCross(int s_first, int s_next, int t_first, int t_next)						  //�����ж�;
{
	long line1, line2;
	pcl::PointXYZ pFirst1, pSecond1, pFirst2, pSecond2;

	pFirst1 = S_polyAfterN.border->points[s_first];
	pSecond1 = S_polyAfterN.border->points[s_next];
	pFirst2 = T_polyAfterN.border->points[t_first];
	pSecond2 = T_polyAfterN.border->points[t_next];

	line1 = pFirst1.x * (pSecond1.y - pFirst2.y) +
		pFirst2.x * (pFirst1.y - pSecond1.y) +
		pSecond1.x * (pFirst2.y - pFirst1.y);
	line2 = pFirst1.x * (pSecond2.y - pFirst2.y) +
		pFirst2.x * (pFirst1.y - pSecond2.y) +
		pSecond2.x * (pFirst2.y - pFirst1.y);
	if (((line1 ^ line2) >= 0) && !(line1 == 0 && line2 == 0))
		return false;

	line1 = pSecond1.x * (pFirst1.y - pSecond2.y) +
		pSecond2.x * (pSecond1.y - pFirst1.y) +
		pFirst1.x * (pSecond2.y - pSecond1.y);
	line2 = pSecond1.x * (pFirst2.y - pSecond2.y) +
		pSecond2.x * (pSecond1.y - pFirst2.y) +
		pFirst2.x * (pSecond2.y - pSecond1.y);
	if (((line1 ^ line2) >= 0) && !(line1 == 0 && line2 == 0))
		return false;

	return true;
}
bool GetCrossPoint(int s_first, int s_next, int t_first, int t_next, float &x, float &y)		  //�������߶εĽ���;
{
	pcl::PointXYZ p1, p2, q1, q2;
	p1 = S_polyAfterN.border->points[s_first];
	p2 = S_polyAfterN.border->points[s_next];
	q1 = T_polyAfterN.border->points[t_first];
	q2 = T_polyAfterN.border->points[t_next];

	if (IsRectCross(s_first, s_next, t_first, t_next))
	{
		if (IsLineSegmentCross(s_first, s_next, t_first, t_next))
		{
			float tmpLeft, tmpRight;

			tmpLeft = (q2.x - q1.x) * (p1.y - p2.y) - (p2.x - p1.x) * (q1.y - q2.y);
			tmpRight = (p1.y - q1.y) * (p2.x - p1.x) * (q2.x - q1.x) + q1.x * (q2.y - q1.y) * (p2.x - p1.x) - p1.x * (p2.y - p1.y) * (q2.x - q1.x);

			x = tmpRight / tmpLeft;

			tmpLeft = (p1.x - p2.x) * (q2.y - q1.y) - (p2.y - p1.y) * (q1.x - q2.x);
			tmpRight = p2.y * (p1.x - p2.x) * (q2.y - q1.y) + (q2.x - p2.x) * (q2.y - q1.y) * (p1.y - p2.y) - q2.y * (q1.x - q2.x) * (p2.y - p1.y);
			y = tmpRight / tmpLeft;
			return true;
		}
	}
	return false;
}

bool IsPointInPolygon(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud, const pcl::PointXYZ &pt)		  //�жϵ��ǲ����ڶ�����ڲ�; 
{
	int i, j;
	bool c = false;

	for (i = 0, j = cloud->size() - 1; i < cloud->size(); j = i++)
	{
		if ((((cloud->points[i].y <= pt.y) && (pt.y < cloud->points[j].y)) || ((cloud->points[j].y <= pt.y) && (pt.y < cloud->points[i].y)))
			&& (pt.x < (cloud->points[j].x - cloud->points[i].x) * (pt.y - cloud->points[i].y) / (cloud->points[j].y - cloud->points[i].y) + cloud->points[i].x))
		{
			c = !c;
		}
	}
	return c;
}

void ClockwiseSortPoints()																		  //�㼯����;
{
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointXYZ center;
	pcl::PointXYZ p;

	float x = 0, y = 0, size;
	size = P_poly.border->size();
	for (int i = 0; i < size; i++)																  //�������ĵ�;
	{
		x += P_poly.border->points[i].x;
		y += P_poly.border->points[i].y;
	}
	center.x = x / size;
	center.y = y / size;
	multimap<float, int> map_indices1;
	multimap<float, int> map_indices2;
	multimap<float, int>::iterator it;
	multimap<float, int>::reverse_iterator rit;

	for (int i = 0; i < P_poly.border->size(); ++i)
	{
		p = P_poly.border->points[i];
		if (p.y >= center.y)
		{
			map_indices1.insert(pair<float, int>(p.x, i));
		}
		else
		{
			map_indices2.insert(pair<float, int>(p.x, i));
		}
	}
	for (it = map_indices1.begin(); it != map_indices1.end(); ++it)
	{
		cloud->points.push_back(P_poly.border->points[it->second]);
	}
	for (rit = map_indices2.rbegin(); rit != map_indices2.rend(); ++rit)
	{
		cloud->points.push_back(P_poly.border->points[rit->second]);
	}
	cloud->height = 1;
	cloud->width = cloud->points.size();
	cloud->is_dense = false;
	cloud->reserve(cloud->height * cloud->width);
	P_poly.border->clear();
	P_poly.border = cloud;
}

bool PublicArea()																				  //��ȡ��������ι�������;
{
	float x, y;
	pcl::PointXYZ p;
	for (int i = 0; i < S_polyAfterN.border->points.size(); i++)								  //�����αߵĽ���;
	{

		int S_poly_next_idx = (i + 1) % S_polyAfterN.border->points.size();

		for (int j = 0; j < T_polyAfterN.border->points.size(); j++)
		{
			int T_poly_next_idx = (j + 1) % T_polyAfterN.border->points.size();
			if (GetCrossPoint(i, S_poly_next_idx, j, T_poly_next_idx, x, y))
			{
				p.x = x;
				p.y = y;
				p.z = 0;
				P_poly.border->points.push_back(p);
			}
		}
	}

	for (int i = 0; i < S_polyAfterN.border->points.size(); i++)
	{
		p = S_polyAfterN.border->points[i];
		if (IsPointInPolygon(T_polyAfterN.border, p))
		{
			P_poly.border->points.push_back(p);
		}
	}
	for (int i = 0; i < T_polyAfterN.border->points.size(); i++)
	{
		p = T_polyAfterN.border->points[i];
		if (IsPointInPolygon(S_polyAfterN.border, p))
		{
			P_poly.border->points.push_back(p);
		}
	}

	if (P_poly.border->size() <= 0)
		return false;
	ClockwiseSortPoints();																		  //�㼯����;
	return true;
}

float GetPolyArea(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud)									  //��ö�������;
{
	int Count;
	float area = 0;

	Count = cloud->points.size();

	for (int i = 0; i < Count; i++)
	{
		area = area + (cloud->points[i].x * cloud->points[(i + 1) % Count].y - cloud->points[(i + 1) % Count].x * cloud->points[i].y);
	}
	return fabs(area / 2);
}

void Match()
{

	float sum = 0;
	float ratio;
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
	P_poly.border = cloud;
	float S_Area = 0;
	float T_Area = 0;
	float P_Area = 0;
	for (int i = 0; i < 4; i++)
	{
		T_polyAfterN = PolyAfterN[i];
		P_poly.border->clear();
		if (PublicArea())
		{
			S_Area = GetPolyArea(S_polyAfterN.border);
			T_Area = GetPolyArea(T_polyAfterN.border);
			P_Area = GetPolyArea(P_poly.border);

			if (S_Area >= T_Area)
			{
				ratio = P_Area / S_Area;
			}
			else
				ratio = P_Area / T_Area;

			if (sum <= ratio)
			{
				sum = ratio;
				p_index = i;
			}
		}

	}
	if (sum >= sigma)
	{
		cout << "match success! " << endl;
		cout << "public area ratio is " << sum << endl;
		cout << "source_index = " << source_index << endl;
		cout << "target_index = " << target_index << endl;
		cout << "p_index = " << p_index << endl;
		cout << endl;
		S_final_index.push_back(source_index);
		T_final_index.push_back(target_index);
		IsMatch = true;
		return;
	}
	else
	{
		cout << "public area ratio is " << sum << " match fail...." << endl;
		cout << "match continue...\n" << endl;
	}
}

/***************************************************************************************************************************************/
/*����任����;*/
void registration()
{

	pcl::PointXYZ s_center;
	pcl::PointXYZ t_center;

	pcl::PointCloud<pcl::PointXYZ>::Ptr output(new pcl::PointCloud<pcl::PointXYZ>);

	cout << "start to get transformation_matrix " << endl;
	float s_size, t_size;
	for (int k = 0; k < S_final_index.size(); ++k)
	{
		float x1 = 0, y1 = 0;
		float x2 = 0, y2 = 0;
		float z1 = 0, z2 = 0;
		s_size = source_polygon[S_final_index[k]].border->size();
		t_size = target_polygon[T_final_index[k]].border->size();
		for (int i = 0; i < s_size; i++)                                                          //����Դ��������ĵ�;
		{
			x1 += source_polygon[S_final_index[k]].border->points[i].x;
			y1 += source_polygon[S_final_index[k]].border->points[i].y;
			z1 += source_polygon[S_final_index[k]].border->points[i].z;
		}
		s_center.x = x1 / s_size;
		s_center.y = y1 / s_size;
		s_center.z = z1 / s_size;

		S_center->points.push_back(s_center);

		for (int j = 0; j < t_size; j++)                                                          //����Ŀ���������ĵ�;
		{
			x2 += target_polygon[T_final_index[k]].border->points[j].x;
			y2 += target_polygon[T_final_index[k]].border->points[j].y;
			z2 += target_polygon[T_final_index[k]].border->points[j].z;
		}
		t_center.x = x2 / t_size;
		t_center.y = y2 / t_size;
		t_center.z = z2 / t_size;
		T_center->points.push_back(t_center);
	}

	pcl::registration::TransformationEstimationSVD<pcl::PointXYZ, pcl::PointXYZ>SVD;
	SVD.estimateRigidTransformation(*S_center, *T_center, transformation_matrix);
	pcl::transformPointCloud(*source_cloud_border, *output, transformation_matrix);
	cout << "transformation_matrix : " << endl;
	cout << transformation_matrix << endl;

	viewer.removeAllPointClouds();
	viewer.addPointCloud(output, "source");
	viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 1.0, 0, 0, "source");
	viewer.addPointCloud(target_cloud_border, "target");
	viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 1.0, 1.0, 0, "target");
}
/***************************************************************************************************************************************/
/*��ʼ��׼����;*/
void registration_cloud()
{
	pcl::PointCloud<pcl::PointXYZ>::Ptr output(new pcl::PointCloud<pcl::PointXYZ>);

	cout << "Now,let's begin to registration! please wait a moment..." << endl;
	pcl::transformPointCloud(*source_cloud, *output, transformation_matrix);
	*source_cloud = *output;


	viewer.removeAllPointClouds();
	viewer.addPointCloud(source_cloud, "source");
	viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 1.0, 0, 0, "source");
	viewer.addPointCloud(target_cloud, "target");
	viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 0, 1.0, 0, "target");


}
/***************************************************************************************************************************************/
void icp()
{
	//std::cout << transform << std::endl;
	//std::cout << "icp start...." << std::endl;
	//pcl::IterativeClosestPoint<pcl::PointXYZ, pcl::PointXYZ> icp;
	//pcl::search::KdTree<pcl::PointXYZ>::Ptr tree1(new pcl::search::KdTree<pcl::PointXYZ>);
	//pcl::PointCloud<pcl::PointXYZ>::Ptr output(new pcl::PointCloud<pcl::PointXYZ>);
	//tree1->setInputCloud(source_cloud);
	//pcl::search::KdTree<pcl::PointXYZ>::Ptr tree2(new pcl::search::KdTree<pcl::PointXYZ>);
	//tree2->setInputCloud(target_cloud);
	//icp.setSearchMethodSource(tree1);
	//icp.setSearchMethodTarget(tree2);
	//icp.setInputSource(source_cloud);
	//icp.setInputTarget(target_cloud);
	//icp.setMaxCorrespondenceDistance(0.05);   //�����ڴ˾���֮��ĵ�;
	//icp.setTransformationEpsilon(1e-10);     //��2��Լ�������ֵһ����Ϊ1e-6���߸�С;
	//icp.setEuclideanFitnessEpsilon(0.005);   //��3��Լ����ǰ�����ε������Ĳ�ֵ;
	//icp.setMaximumIterations(100);          //��1��Լ������������;
	//icp.align(*reg_result);
	//Eigen::Matrix4f transform4to3;
	////*output = *source_cloud;
	//transform4to3 = icp.getFinalTransformation();

	//cout << "transformation_matrix : " << endl;
	//cout << transformation_matrix << endl;

	//pcl::transformPointCloud(*target_cloud, *output, transform4to3);

	////*reg_result = *output + *target_cloud;
	//double a = icp.getFitnessScore();
	//double b = icp.getEuclideanFitnessEpsilon();
	//cout << icp.hasConverged() <<"score" << a << '\t' <<  b << endl;


	//viewer.removeAllPointClouds();
	//viewer.addPointCloud(output, "cloud");
	//viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 1.0, 0, 0, "cloud");
	//viewer.addPointCloud(target_cloud, "cloud1");
	//viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 0, 1.0, 0, "cloud1");

	//std::cout << "icp finish...." << std::endl;

}