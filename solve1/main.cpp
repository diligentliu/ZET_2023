#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <queue>
#include <climits>

using namespace std;

string dataPath = "../data";
string flowFile = "flow.txt";
string portFile = "port.txt";
string resultFile = "result.txt";

class Flow {
public:
	int id;
	int portId;
	int bandwidth;
	int startTime;
	int beginTime;
	int endTime;
	int sendTime;
	double speed1;
	double speed2;
	double compose;

	explicit Flow(int id = -1, int bandwidth = 0, int startTime = 0, int sendTime = 0);
	bool isNull() const;
	void setBeginTime(int bt);
	void setEndTime(int bt);
	bool operator<(const Flow &other) const;
	bool operator>(const Flow &other) const;
	bool operator==(const Flow &other) const;
};

Flow::Flow(int id, int bandwidth, int startTime, int sendTime) {
	this->id = id;
	this->portId = -1;
	this->bandwidth = bandwidth;
	this->startTime = startTime;
	this->sendTime = sendTime;
	this->beginTime = 0;
	this->endTime = INT_MAX;
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

// 建立大顶堆所需比较类
class CompareAsSendTime {
public:
	bool operator()(Flow &flow1, Flow &flow2) {
		return flow1.sendTime < flow2.sendTime;
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
int binary_search(vector<Port> &ports, int value) {
	int left = 0, right = ports.size() - 1;
	int ans = -1;
	while (left <= right) {
		int mid = (right + left) / 2;
		if (ports[mid].remainBandwidth >= value) {
			ans = mid;
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	return ans;
}

int transfer(list<Flow> flows, vector<Port> ports, vector<vector<int>> &results) {
	unsigned portNum = ports.size();
	int resultPos = 0;
	// 端口按照剩余带宽大小进行升序排列，并记录最大的剩余带宽，用来提前判断流有没有可以发送的端口
	sort(ports.begin(), ports.end(), less<>());
	int maxRemainBandwidth = ports[portNum - 1].remainBandwidth;
	// 当前时间
	int time = 0;
	// 发送所有的流所用时间
	int maxTime = 0;
	// 空流变量
	Flow temp;
	Flow flow, flowAtPort, flowAtDispatch;
	// 保存端口正在发送的流，所有端口共用
	priority_queue<Flow, vector<Flow>, greater<>> min_heap;
	// 缓存区，按照发送所需时间降序排列
	priority_queue<Flow, vector<Flow>, CompareAsSendTime> dispatch;
	while (!flows.empty() || !dispatch.empty()) {
		flow = (!flows.empty() ? flows.front() : temp);
		flowAtPort = (!min_heap.empty() ? min_heap.top() : temp);
		// 更新端口，查看端口有无已经发送完毕的流，并更新端口剩余带宽、排序、保存最大剩余带宽
		while (!flowAtPort.isNull() && flowAtPort.endTime == time) {
			for (auto &port: ports) {
				if (port.id == flowAtPort.portId) {
					port.modifyRemain(-flowAtPort.bandwidth);
					min_heap.pop();
					sort(ports.begin(), ports.end(), greater<>());
					maxRemainBandwidth = ports[0].remainBandwidth;
					flowAtPort = (!min_heap.empty() ? min_heap.top() : temp);
					break;
				}
			}
		}
		// 查看是否有流进入设备，若有进入放入缓存区堆中
		while (!flow.isNull() && flow.startTime <= time) {
			dispatch.push(flow);
			flows.pop_front();
			flow = (!flows.empty() ? flows.front() : temp);
		}
		// 发送缓存区中的流，选择剩余带宽大于该流的最小端口
		while (!dispatch.empty()) {
			flowAtDispatch = dispatch.top();
			if (flowAtDispatch.bandwidth <= maxRemainBandwidth) {
				int i = binary_search(ports, flowAtDispatch.bandwidth);
				flowAtDispatch.setBeginTime(time);
				flowAtDispatch.setEndTime(time);
				flowAtDispatch.portId = ports[i].id;
				// fprintf(fpWrite, "%d,%d,%d\n", flowAtDispatch.id, flowAtDispatch.portId, flowAtDispatch.beginTime);
				// cout << flowAtDispatch.id << "," << flowAtDispatch.portId << "," << flowAtDispatch.beginTime << endl;
				results[resultPos][0] = flowAtDispatch.id;
				results[resultPos][1] = flowAtDispatch.portId;
				results[resultPos][2] = flowAtDispatch.beginTime;
				resultPos++;
				maxTime = max(maxTime, flowAtDispatch.endTime);
				min_heap.push(flowAtDispatch);
				ports[i].modifyRemain(flowAtDispatch.bandwidth);
				sort(ports.begin(), ports.end(), greater<>());
				maxRemainBandwidth = ports[0].remainBandwidth;
				dispatch.pop();
			} else {
				break;
			}
		}
		++time;
	}
	return maxTime;
}

// 写入文件
void write_file(const char *outFilePath, vector<vector<int>> results, const unsigned long &num) {
	FILE *fpWrite = fopen(outFilePath, "w");
	for (int i = 0; i < num; ++i) {
		fprintf(fpWrite, "%d,%d,%d\n", results[i][0], results[i][1], results[i][2]);
	}
	fclose(fpWrite);
}

int main() {
	int dirNum = 0;
	auto lambda = [](Flow &first, Flow &second) {
		return first.startTime < second.startTime;
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
		flows.sort(lambda);

		auto flowsNum = flows.size();

		vector<vector<int>> results(flowsNum, vector<int>(3));
		transfer(flows, ports, results);

		write_file(resultsFilePath.c_str(), results, flowsNum);
		dirNum++;
	}
}
