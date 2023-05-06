#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <queue>
#include <climits>

using namespace std;

string dataPath = "../testData_1";
string flowFile = "flow.txt";
string portFile = "port.txt";
string resultFile = "result2.txt";

class Flow {
public:
	int id;
	int portId;
	int bandwidth;
	int startTime;
	int beginTime;
	int endTime;
	int sendTime;
	double speed;
	double compose;

	explicit Flow(int id = -1, int bandwidth = 0, int startTime = 0, int sendTime = 0);
	bool isNull() const;
	void setBeginTime(int bt);
	void setEndTime(int bt);
	bool operator<(const Flow &other) const;
	bool operator>(const Flow &other) const;
	bool operator==(const Flow &other) const;
	friend ostream &operator<<(ostream &out, Flow &flow);
};

Flow::Flow(int id, int bandwidth, int startTime, int sendTime) {
	this->id = id;
	this->portId = -1;
	this->bandwidth = bandwidth;
	this->startTime = startTime;
	this->sendTime = sendTime;
	this->beginTime = 0;
	this->endTime = INT_MAX;
	this->speed = (double) (sendTime) / (double) (bandwidth);
}

bool Flow::isNull() const {
	return id == -1;
}

void Flow::setBeginTime(int bt) {
	this->beginTime = bt;
}

void Flow::setEndTime(int bt) {
	this->endTime = bt + sendTime;
}

bool Flow::operator<(const Flow &other) const {
	return this->endTime < other.endTime;
}

bool Flow::operator>(const Flow &other) const {
	return this->endTime > other.endTime;
}

bool Flow::operator==(const Flow &other) const {
	return this->endTime == other.endTime;
}

ostream &operator<<(ostream &out, Flow &flow) {
	out << flow.id << "," << flow.bandwidth << "," << flow.startTime << "," << flow.sendTime << "," << flow.speed;
	return out;
}

class CompareAsCompose {
public:
	bool operator()(Flow &flow1, Flow &flow2) {
		return flow1.compose > flow2.compose;
	}
};

class Port {
public:
	int id;
	int bandwidth;
	int remainBandwidth;

	Port(int id, int bandwidth);
	bool modifyRemain(int bw);
	bool operator<(const Port &other) const;
	bool operator==(const Port &other) const;
	bool operator>(const Port &other) const;
	friend ostream &operator<<(ostream &out, Port &port);
};

Port::Port(int id, int bandwidth) {
	this->id = id;
	this->bandwidth = bandwidth;
	this->remainBandwidth = bandwidth;
}

bool Port::modifyRemain(int bw) {
	if (bw > remainBandwidth) {
		return false;
	}
	remainBandwidth -= bw;
	return true;
}

bool Port::operator<(const Port &other) const {
	return this->remainBandwidth < other.remainBandwidth;
}

bool Port::operator>(const Port &other) const {
	return this->remainBandwidth > other.remainBandwidth;
}

bool Port::operator==(const Port &other) const {
	return this->remainBandwidth == other.remainBandwidth;
}

ostream &operator<<(ostream &out, Port &port) {
	out << port.id << "," << port.bandwidth;
	return out;
}

void loadFlow(const char *filePath, list<Flow> &flows) {
	FILE *fpRead = fopen(filePath, "r");
	if (fpRead == nullptr) {
		return;
	}
	int id;
	int bandwidth;
	int startTime;
	int sendTime;
	// 忽略第一行
	fscanf(fpRead, "%*[^\n]%*c");
	while (fscanf(fpRead, "%d,%d,%d,%d\n", &id, &bandwidth, &startTime, &sendTime) != EOF) {
		flows.emplace_back(id, bandwidth, startTime, sendTime);
	}
}

void loadPort(const char *filePath, vector<Port> &posts) {
	FILE *fpRead = fopen(filePath, "r");
	if (fpRead == nullptr) {
		return;
	}
	int id;
	int bandwidth;
	// 忽略第一行
	fscanf(fpRead, "%*[^\n]%*c");
	while (fscanf(fpRead, "%d,%d\n", &id, &bandwidth) != EOF) {
		posts.emplace_back(id, bandwidth);
	}
}

// 二分查找 posts 中第一个大于 bandwidth 的索引
int binary_search(vector<Port> &posts, int bandwidth) {
	int n = posts.size();
	int left = 0, right = n - 1;
	int res = -1;
	while (left <= right) {
		int mid = left + (right - left) / 2;
		if (posts[mid].remainBandwidth >= bandwidth) {
			res = mid;
			right = mid - 1;
		} else {
			left = mid + 1;
		}
	}
	return res;
}

int transfer(list<Flow> flows, vector<Port> ports, const string &resultsFile, double &a, double &b) {
	FILE *fpWrite = fopen(resultsFile.c_str(), "w");
	unsigned portNum = ports.size();
	sort(ports.begin(), ports.end(), less<>());
	int maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
	int time = 0;
	int maxTime = 0;
	Flow temp;
	Flow flow, flowAtPort, flowAtDispatch;
	priority_queue<Flow, vector<Flow>, greater<>> min_heap;
	priority_queue<Flow, vector<Flow>, CompareAsCompose> dispatch;
	while (!flows.empty() || !dispatch.empty()) {
		flow = (!flows.empty() ? flows.front() : temp);
		flow.compose = flow.speed + a * double(flow.sendTime) + b * double(flow.bandwidth);
		flowAtPort = (!min_heap.empty() ? min_heap.top() : temp);
		while (!flowAtPort.isNull() && flowAtPort.endTime == time) {
			for (auto &port: ports) {
				if (port.id == flowAtPort.portId) {
					port.modifyRemain(-flowAtPort.bandwidth);
					min_heap.pop();
					sort(ports.begin(), ports.end(), less<>());
					maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
					flowAtPort = (!min_heap.empty() ? min_heap.top() : temp);
					break;
				}
			}
		}
		while (!flow.isNull() && flow.startTime <= time) {
			dispatch.push(flow);
			flows.pop_front();
			flow = (!flows.empty() ? flows.front() : temp);
			flow.compose = flow.speed + a * double(flow.sendTime) + b * double(flow.bandwidth);
		}
		while (!dispatch.empty()) {
			flowAtDispatch = dispatch.top();
			if (flowAtDispatch.bandwidth <= maxRemainBandwidth) {
				int i = binary_search(ports, flowAtDispatch.bandwidth);
				flowAtDispatch.setBeginTime(time);
				flowAtDispatch.setEndTime(time);
				flowAtDispatch.portId = ports[i].id;
				// fprintf(fpWrite, "%d,%d,%d\n", flowAtDispatch.id, flowAtDispatch.portId, flowAtDispatch.beginTime);
				// cout << flowAtDispatch.id << "," << flowAtDispatch.portId << "," << flowAtDispatch.beginTime << endl;
				maxTime = max(maxTime, flowAtDispatch.endTime);
				min_heap.push(flowAtDispatch);
				ports[i].modifyRemain(flowAtDispatch.bandwidth);
				sort(ports.begin(), ports.end(), less<>());
				maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
				dispatch.pop();
			} else {
				break;
			}
		}
		++time;
	}
	return maxTime;
}

void write_file(const char *outFilePath, vector<vector<double>> &results, const int &num) {
	FILE *fpWrite = fopen(outFilePath, "w");
	for (int i = 0; i < num; ++i) {
		fprintf(fpWrite, "%lf,%lf,%lf\n", results[i][0], results[i][1], results[i][2]);
	}
	fclose(fpWrite);
}

int main() {
	int dirNum = 0;
	auto lambda = [&](Flow first, Flow second) {
		if (first.startTime != second.startTime) {
			return first.startTime < second.startTime;
		} else if (first.bandwidth != second.bandwidth) {
			return first.bandwidth < second.bandwidth;
		} else {
			return first.sendTime < second.sendTime;
		}
	};
	while (true) {
		string flowsFilePath;
		flowsFilePath.append(dataPath).append("/").append(to_string(dirNum)).append("/").append(flowFile);
		string portsFilePath;
		portsFilePath.append(dataPath).append("/").append(to_string(dirNum)).append("/").append(portFile);
		string resultsFilePath;
		resultsFilePath.append(dataPath).append("/").append(to_string(dirNum)).append("/").append(resultFile);
		list<Flow> flows;
		vector<Port> ports;

		FILE *f = fopen(portsFilePath.c_str(), "r");
		if (f == nullptr) {
			return 0;
		}
		fclose(f);
		loadFlow(flowsFilePath.c_str(), flows);
		loadPort(portsFilePath.c_str(), ports);

		FILE *fpWrite = fopen(resultsFilePath.c_str(), "w");
		flows.sort([](Flow &first, Flow &second) {
			return first.startTime < second.startTime;
		});
		int ret = INT_MAX;
		double pa;
		double pb;
		vector<vector<double>> results(0, vector<double>(3));
		for (int i = -100; i <= 0; ++i) {
			for (int j = -20; j < 20; ++j) {
				double a = double (i) / 10;
				double b = double (j) / 10;
				int ret1 = transfer(flows, ports, resultsFilePath, a, b);

				if (ret1 <= 1140) {
					vector<double> vec = {a, b, double(ret1)};
					results.push_back(vec);
				}

				cout << a << "," << b << "," << ret1 << endl;

				if (ret1 < ret) {
					pa = a;
					pb = b;
					ret = ret1;
				}
			}
		}
		cout << pa << "," << pb << "," << ret << endl;
		cout << endl;
		for (auto result : results) {
			cout << result[0] << "," << result[1] << "," << result[2] << endl;
		}
		fclose(fpWrite);
		dirNum++;
	}
}
