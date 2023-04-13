import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.*;
import java.util.Queue;

class Flow implements Comparable<Flow> {
	public int id;
	public int bandwidth;
	public int startTime;
	public int beginTime;
	public int endTime;
	public int sendTime;
	public double speed;

	public Flow(int id, int bandwidth, int startTime, int sendTime) {
		this.id = id;
		this.bandwidth = bandwidth;
		this.startTime = startTime;
		this.sendTime = sendTime;
		this.beginTime = 0;
		this.speed = (double) bandwidth / (double) sendTime;
	}

	public Flow() { this.id = -1; }

	public boolean isNull() {
		return id == -1;
	}

	public void setBeginTime(int beginTime) {
		this.beginTime = beginTime;
	}

	public void setEndTime(int beginTime) {
		this.endTime = beginTime + sendTime;
	}

	public int compareTo(Flow other) {
		return Integer.compare(endTime, other.endTime);
	}

	@Override
	public String toString() {
		return id + "," + bandwidth + "," + startTime + "," + sendTime + "," + speed;
	}
}

class FlowComparator implements Comparator<Flow> {
	@Override
	public int compare(Flow first, Flow second) {
		if (first.startTime != second.startTime) {
			return Integer.compare(first.startTime, second.startTime);
		} else if (first.bandwidth != second.bandwidth) {
			return Integer.compare(first.bandwidth, first.bandwidth);
		} else {
			return Integer.compare(second.sendTime, first.sendTime);
		}
	}
}
class Port implements Comparable<Port> {
	public int id;
	public int bandwidth;
	public int remainBandwidth;
	public Port(int id, int bandwidth) {
		this.id = id;
		this.bandwidth = bandwidth;
		this.remainBandwidth = bandwidth;
	}
	public Port() {
		this.id = -1;
	}
	public int compareTo(Port other) {
		return Integer.compare(bandwidth, other.bandwidth);
	}
	@Override
	public String toString() {
		return id + "," + bandwidth;
	}
}

class Result {
	public int flowId;
	public int portId;
	public int startSendTime;
	public Result(int flowId, int portId, int startSendTime) {
		this.flowId = flowId;
		this.portId = portId;
		this.startSendTime = startSendTime;
	}
	@Override
	public String toString() {
		return flowId + "," + portId + "," + startSendTime;
	}
}
public class Main {
	public static String dataPath = "../data";
	public static String flowFile = "flow.txt";
	public static String portFile = "port.txt";
	public static String resultFile = "result.txt";
	public static int loadFlow(String filePath, PriorityQueue<Flow> flows) throws FileNotFoundException {
		int total = 0;
		int[] tempInt = new int[4];
		try (Scanner sc = new Scanner(new FileReader(filePath))) {
			sc.nextLine();
			while (sc.hasNextLine()) {
				String line = sc.nextLine();
				String[] tempString = line.split(",");
				for (int i = 0; i < 4; ++i) {
					tempInt[i] = Integer.parseInt(tempString[i]);
				}
				total += tempInt[1];
				flows.add(new Flow(tempInt[0], tempInt[1], tempInt[2], tempInt[3]));
			}
		}
		return total;
	}
	public static int loadPort(String filePath, ArrayList<Port> ports) throws FileNotFoundException {
		int total = 0;
		int[] tempInt = new int[2];
		try (Scanner sc = new Scanner(new FileReader(filePath))) {
			sc.nextLine();
			while (sc.hasNextLine()) {
				String line = sc.nextLine();
				String[] tempString = line.split(",");
				for (int i = 0; i < 2; ++i) {
					tempInt[i] = Integer.parseInt(tempString[i]);
				}
				total += tempInt[1];
				ports.add(new Port(tempInt[0], tempInt[1]));
			}
		}
		return total;
	}
	public static void getPortWeight(ArrayList<Integer> portsWeight, ArrayList<Port> ports, int postsTotalBandwidth, int flowsTotalBandwidth) {
		int portNum = ports.size();
		for (int i = 0; i < portNum; ++i) {
			portsWeight.add((int) (flowsTotalBandwidth * ((double) ports.get(i).bandwidth / (double) postsTotalBandwidth)));
		}
	}
	private static void getQueue(Queue<Flow>[] queues, ArrayList<Integer> queueBandwidth, ArrayList<Integer> portsWeight, ArrayList<Port> ports, PriorityQueue<Flow> flows) {
		int queueNum = ports.size();
		int time = 0;
		int queuePos = 0;
		while (!flows.isEmpty()) {
			Flow flow = flows.peek();
			if (flow.startTime == time && (Integer)queueBandwidth.get(queuePos) < (Integer) portsWeight.get(queuePos)) {
				queues[queuePos].add(flow);
				queueBandwidth.set(queuePos, (Integer)queueBandwidth.get(queuePos) + flow.bandwidth);
				queuePos = (queuePos + 1) % queueNum;
				flows.poll();
			} else if (flow.startTime == time && (Integer)queueBandwidth.get(queuePos) >= (Integer) portsWeight.get(queuePos)) {
				queuePos = queuePos + 1;
				if (queuePos == queueNum) {
					queuePos--;
					queues[queuePos].add(flow);
					queueBandwidth.set(queuePos, (Integer)queueBandwidth.get(queuePos) + flow.bandwidth);
					flows.poll();
				}
			} else if (flow.startTime > time) {
				time++;
			}
		}
	}
	public static void main(String[] args) throws FileNotFoundException {
		int dirNum = 0;
		String flowsFile = dataPath + "/" + dirNum + "/" + flowFile;
		String portsFile = dataPath + "/" + dirNum + "/" + portFile;

		PriorityQueue<Flow> flows = new PriorityQueue<>(new FlowComparator());
		int flowsTotalBandwidth = loadFlow(flowsFile, flows);
		// while (!flows.isEmpty()) {
		// 	Flow flow = flows.peek();
		// 	System.out.println(flow);
		// 	flows.poll();
		// }

		ArrayList<Port> ports = new ArrayList<>();
		int postsTotalBandwidth = loadPort(portsFile, ports);
		Collections.sort(ports);
		// for (Port port : ports) {
		// 	System.out.println(port);
		// }

		int flowsNum = flows.size();
		int[][] results = new int[flowsNum][3];

		int queueNum = ports.size();
		Queue<Flow>[] queues = new Queue[queueNum];
		for (var queue : queues) {
			queue = new LinkedList<Flow>();
		}
		ArrayList<Integer> queueBandwidth = new ArrayList<>(queueNum);
		ArrayList<Integer> portsWeight = new ArrayList<>(queueNum);
		for (int i = 0; i < queueNum; ++i) {
			queueBandwidth.add(0);
		}
		getPortWeight(portsWeight, ports, postsTotalBandwidth, flowsTotalBandwidth);
		getQueue(queues, queueBandwidth, portsWeight, ports, flows);
		while (!queues[0].isEmpty()) {
			Flow flow = queues[0].peek();
			System.out.println(flow);
			queues[0].poll();
		}
		for (var queue : queues) {
			System.out.println(queue.size());
		}
	}



}
