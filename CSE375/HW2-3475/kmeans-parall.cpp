// Implementation of the KMeans Algorithm
// reference: https://github.com/marcoscastro/kmeans

#include <iostream>
#include <vector>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <chrono>
#include "oneapi-tbb-2021.8.0/include/oneapi/tbb.h"
#include "oneapi-tbb-2021.8.0/include/oneapi/tbb/parallel_for.h"
#include "oneapi-tbb-2021.8.0/include/oneapi/tbb/blocked_range.h"
#include "oneapi-tbb-2021.8.0/include/oneapi/tbb/mutex.h"
#include <mutex>
#include <shared_mutex>

using namespace std;

class Point
{
private:
	int id_point, id_cluster;
	vector<double> values;
	int total_values;
	string name;

public:
	Point(int id_point, vector<double>& values, string name = "")
	{
		this->id_point = id_point;
		total_values = values.size();

		for(int i = 0; i < total_values; i++)
			this->values.push_back(values[i]);

		this->name = name;
		id_cluster = -1;
	}

	int getID()
	{
		return id_point;
	}

	void setCluster(int id_cluster)
	{
		this->id_cluster = id_cluster;
	}

	int getCluster()
	{
		return id_cluster;
	}

	double getValue(int index)
	{
		return values[index];
	}

	int getTotalValues()
	{
		return total_values;
	}

	void addValue(double value)
	{
		values.push_back(value);
	}

	string getName()
	{
		return name;
	}
};

class Cluster
{
private:
	int id_cluster;
	vector<double> central_values;
    vector<Point> points;
	// tbb::mutex mtx;



public:
	
	Cluster(int id_cluster, Point point)
	{
		this->id_cluster = id_cluster;

		int total_values = point.getTotalValues();

		for(int i = 0; i < total_values; i++)
			central_values.push_back(point.getValue(i));

		points.push_back(point);
	}

	Cluster(const Cluster& c)
	{
		this->id_cluster = c.id_cluster;
		this->central_values = c.central_values;
		this->points = c.points;
	}

	Cluster& operator=(const Cluster& c)
	{
		if(this != &c)
		{
			this->id_cluster = c.id_cluster;
			this->central_values = c.central_values;
			this->points = c.points;
		}
		return *this;
	}
	// Cluster & operator=(const Cluster&) = delete;

	void addPoint(Point point)
	{
		// std::lock_guard<tbb::mutex> lock(mtx);
		points.push_back(point);
	}

	bool removePoint(int id_point)
	{
		// std::lock_guard<tbb::mutex> lock(mtx);
		auto it =std::remove_if(points.begin(), points.end(), 
								[id_point](Point p){return p.getID() == id_point;});
		if(it != points.end())
		{
			points.erase(it, points.end());
			return true;
		}
		return false;
		// int total_points = points.size();
		

		// for(int i = 0; i < total_points; i++)
		// {
		// 	if(points[i].getID() == id_point)
		// 	{
		// 		points.erase(points.begin() + i);
		// 		return true;
		// 	}
		// }
		// return false;
	}

	double getCentralValue(int index)
	{
		
		return central_values[index];
	}

	void setCentralValue(int index, double value)
	{
		
		central_values[index] = value;
	}

	Point getPoint(int index)
	{

		return points[index];
	}

	int getTotalPoints()
	{
		
		return points.size();
	}

	int getID()
	{
		return id_cluster;
	}
};



// use the lock- mutex to protect the shared data
// one for each cluster
class KMeans
{
private:
	int K; // number of clusters
	int total_values, total_points, max_iterations;
	vector<Cluster> clusters;
	vector<tbb::mutex> cluster_mtx; // for each cluster 

	// return ID of nearest center (uses euclidean distance)
	int getIDNearestCenter(Point point)
	{
		double sum = 0.0, min_dist;
		int id_cluster_center = 0;

		for(int i = 0; i < total_values; i++)
		{
			sum += pow(clusters[0].getCentralValue(i) -
					   point.getValue(i), 2.0);
		}

		min_dist = sqrt(sum);

		for(int i = 1; i < K; i++)
		{
			double dist;
			sum = 0.0;

			for(int j = 0; j < total_values; j++)
			{
				sum += pow(clusters[i].getCentralValue(j) -
						   point.getValue(j), 2.0);
			}

			dist = sqrt(sum);

			if(dist < min_dist)
			{
				min_dist = dist;
				id_cluster_center = i;
			}
		}

		return id_cluster_center;
	}

public:
	

	KMeans(int K, int total_points, int total_values, int max_iterations)
	{
		this->K = K;
		this->total_points = total_points;
		this->total_values = total_values;
		this->max_iterations = max_iterations;

	}
	
	// parallel version
	void parallelAssociatePoints(vector<Point> &points, int num_threads)
	{
		 
    	tbb::global_control gc(tbb::global_control::max_allowed_parallelism, num_threads);
		tbb::parallel_for(tbb::blocked_range<int>(0,total_points), [&](const tbb:: blocked_range<int>& r)
		{
			for(int i = r.begin(); i < r.end(); i++)
			{
				int id_nearest_center = getIDNearestCenter(points[i]);
				points[i].setCluster(id_nearest_center);

				std::lock_guard<tbb::mutex> lock(cluster_mtx[id_nearest_center]);
				clusters[id_nearest_center].addPoint(points[i]);
			}
		});
	}

	void run(vector<Point> & points, int num_threads)
	{
        auto begin = chrono::high_resolution_clock::now();
        
		if(K > total_points)
			return;

		vector<int> prohibited_indexes;

		// choose K distinct values for the centers of the clusters
		for(int i = 0; i < K; i++)
		{
			while(true)
			{
				int index_point = rand() % total_points;

				if(find(prohibited_indexes.begin(), prohibited_indexes.end(),
						index_point) == prohibited_indexes.end())
				{
					prohibited_indexes.push_back(index_point);
					points[index_point].setCluster(i);
					Cluster cluster(i, points[index_point]);
					clusters.push_back(cluster);
					
					break;
				}
			}
		}
        auto end_phase1 = chrono::high_resolution_clock::now();
        
		int iter = 1;

		while(true)
		{
			bool done = true;

			// associates each point to the nearest center
			parallelAssociatePoints(points,num_threads);
			for(int i = 0; i < total_points; i++)
			{
				int id_old_cluster = points[i].getCluster();
				int id_nearest_center = getIDNearestCenter(points[i]);

				if(id_old_cluster != id_nearest_center)
				{
					if(id_old_cluster != -1){

						clusters[id_old_cluster].removePoint(points[i].getID());
						
					}

					points[i].setCluster(id_nearest_center);

					clusters[id_nearest_center].addPoint(points[i]);
					done = false;
				}
			}

			for(int i = 0; i < K; i++)
			{	
                
				for(int j = 0; j < total_values; j++)
				{
					int total_points_cluster = clusters[i].getTotalPoints();
					double sum = 0.0;

					if(total_points_cluster > 0)
					{
						for(int p = 0; p < total_points_cluster; p++)
							sum += clusters[i].getPoint(p).getValue(j);
						clusters[i].setCentralValue(j, sum / total_points_cluster);
					}
				}
			}

			if(done == true || iter >= max_iterations)
			{
				cout << "Break in iteration " << iter << "\n\n";
				break;
			}

			iter++;
		}
        auto end = chrono::high_resolution_clock::now();

		// shows elements of clusters
		for(int i = 0; i < K; i++)
		{
			int total_points_cluster =  clusters[i].getTotalPoints();

			cout << "Cluster " << clusters[i].getID() + 1 << endl;
			for(int j = 0; j < total_points_cluster; j++)
			{
				cout << "Point " << clusters[i].getPoint(j).getID() + 1 << ": ";
				for(int p = 0; p < total_values; p++)
					cout << clusters[i].getPoint(j).getValue(p) << " ";

				string point_name = clusters[i].getPoint(j).getName();

				if(point_name != "")
					cout << "- " << point_name;

				cout << endl;
			}

			cout << "Cluster values: ";

			for(int j = 0; j < total_values; j++)
				cout << clusters[i].getCentralValue(j) << " ";

			cout << "\n\n";
            cout << "TOTAL EXECUTION TIME = "<<std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count()<<"\n";
            
            cout << "TIME PHASE 1 = "<<std::chrono::duration_cast<std::chrono::microseconds>(end_phase1-begin).count()<<"\n";
            
            cout << "TIME PHASE 2 = "<<std::chrono::duration_cast<std::chrono::microseconds>(end-end_phase1).count()<<"\n";
		}
	}
};

int main(int argc, char *argv[])
{
	srand (time(NULL));

	int total_points, total_values, K, max_iterations, has_name, num_threads;

	cin >> total_points >> total_values >> K >> max_iterations >> has_name >> num_threads;

	vector<Point> points;
	string point_name;

	for(int i = 0; i < total_points; i++)
	{
		vector<double> values;

		for(int j = 0; j < total_values; j++)
		{
			double value;
			cin >> value;
			values.push_back(value);
		}

		if(has_name)
		{
			cin >> point_name;
			Point p(i, values, point_name);
			points.push_back(p);
		}
		else
		{
			Point p(i, values);
			points.push_back(p);
		}
	}
	 // Run the algorithm for different numbers of threads
    for (int num_threads = 1; num_threads <= 20; num_threads++)
    {
		if (num_threads == 1){
			 cout << "Running with " << num_threads << " threads:\n";
			KMeans kmeans(K, total_points, total_values, max_iterations);
			kmeans.run(points, num_threads);
			
			cout << "====================================\n\n";
		}
		else{
			cout << "Running with " << num_threads << " threads:\n";
			KMeans kmeans(K, total_points, total_values, max_iterations);
			kmeans.run(points, num_threads);
			
			cout << "====================================\n\n";
			num_threads++;
		}
       
    }

    return 0;
	KMeans kmeans(K, total_points, total_values, max_iterations);
	kmeans.run(points, num_threads);

	return 0;
}
