package org.apache.hadoop.yarn.dmtk.am;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ConcurrentSkipListSet;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicInteger;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.yarn.api.records.Container;
import org.apache.hadoop.yarn.api.records.ContainerExitStatus;
import org.apache.hadoop.yarn.api.records.ContainerId;
import org.apache.hadoop.yarn.api.records.ContainerState;
import org.apache.hadoop.yarn.api.records.ContainerStatus;
import org.apache.hadoop.yarn.api.records.Priority;
import org.apache.hadoop.yarn.api.records.Resource;
import org.apache.hadoop.yarn.dmtk.DSConstants;
import org.apache.hadoop.yarn.dmtk.am.ContainersManager.Status;
import org.apache.hadoop.yarn.dmtk.am.MyContainer.MyContainerStatus;
import org.apache.hadoop.yarn.client.api.AMRMClient.ContainerRequest;
import org.apache.hadoop.yarn.client.api.async.AMRMClientAsync;
import org.apache.hadoop.yarn.client.api.async.NMClientAsync;
import org.apache.hadoop.yarn.util.Records;
import org.mortbay.jetty.servlet.PathMap;

public class ContainersManager {
	public static enum Status {
		New, StartingServer, StartingWorker, 
		Running,
		Failed, Succeed, Stopped
	}
	
	private static final Log LOG = LogFactory.getLog(ContainersManager.class);
	private int numServers, numWorkers, workerServerPort_;
	private List<MyContainer> pendingServer = new ArrayList<MyContainer>();
	private List<MyContainer> serverBackup = new ArrayList<MyContainer>();
	private List<MyContainer> pendingWorker = new ArrayList<MyContainer>();
	private int failedNum = 0, succeedWorkerNum = 0;
	private int startedServerNum = 0, startedWorkerNum = 0;
	private int processMemory_ = 0, processCores_ = 0;
	private AtomicInteger requestingContainersNum = new AtomicInteger();
	private AMRMClientAsync amRMClient_;
	private HashMap<Container, MyContainer> allocatedContainers_ =
			new HashMap<Container, MyContainer>();
	private HashMap<ContainerId, MyContainer> allocatedContainerIds_ =
			new HashMap<ContainerId, MyContainer>();
	public Status status = Status.New;

	public ContainersManager() {
		
	}
	
	public void PrintLog() {
		LOG.info("ContainerManager.status=" + status
				+ ",pendingServerNum=" + pendingServer.size()
				+ ",pendingWorkerNum=" + pendingWorker.size()
				+ ",allocatedContainersNum=" + allocatedContainers_.size()
				+ ",requestingContainersNum=" + requestingContainersNum.get()
				+ ",startedServerNum=" + startedServerNum
				+ ",startedWorkerNum=" + startedWorkerNum
				+ ",failedNum=" + failedNum
				+ ",succeedWorkerNum=" + succeedWorkerNum);
	}
	
	Set<Container> GetAllContainers() {
		return allocatedContainers_.keySet();
	}
	
	public MyContainer Init(int numServers, int numWorkers,
			int workerServerPort,
			int processMemory,
			int processCores,
			CommandFileGenerator commandFileGenerator,
			NMCallbackHandler containerListener,
			ByteBuffer allTokens,
			Configuration conf,
			NMClientAsync nmClientAsync,
			AMRMClientAsync amRMClient,
			ExecutorService containerLauncher) {
		processMemory_ = processMemory;
		processCores_ = processCores;
		amRMClient_ = amRMClient;
		this.numServers = numServers;
		this.numWorkers = numWorkers;
		workerServerPort_ = workerServerPort;
		requestingContainersNum.set(0);
		for (int i = 0; i < numServers; ++i) {
			MyContainer myContainer = new MyContainer(DSConstants.SERVER,
					i, commandFileGenerator,
					containerListener,
					allTokens,
					conf,
					nmClientAsync,
					containerLauncher);
			pendingServer.add(myContainer);
			serverBackup.add(myContainer);
		}

		for (int i = 1; i < numWorkers; ++i) {
			pendingWorker.add(new MyContainer(DSConstants.WORKER,
					i, commandFileGenerator,
					containerListener,
					allTokens,
					conf,
					nmClientAsync,
					containerLauncher));
		}

		return new MyContainer(DSConstants.WORKER,
					0, commandFileGenerator,
					containerListener,
					allTokens,
					conf,
					nmClientAsync,
					containerLauncher);
	}

	public void Start(int allocateTime, int maxRetryTime, int maxWaitTime) {
		requestingContainersNum.set(0);
		status = Status.StartingServer;
		LOG.info("Starting Servers ...");
		if (Start(allocateTime, maxRetryTime, maxWaitTime,
				DSConstants.SERVER, Status.StartingServer, pendingServer)) {
			LOG.info("Succeed to start all Servers");
		  	LOG.info("Starting Workers ...");
		  	if (Start(allocateTime, maxRetryTime, maxWaitTime,
					DSConstants.WORKER, Status.StartingWorker, pendingWorker)) {
		  		LOG.info("Succeed to start all Workers");
			} else {
				status = Status.Failed;
				LOG.error("Failed to start all the workers");
			}
		} else {
			status = Status.Failed;
			LOG.error("Failed to start all the servers");
		}
	}
	
	private boolean Start(int allocateTime, int maxRetryTime, int maxWaitTime,
			String type, Status workingStatus, List<MyContainer> pending) {
	    try {
	    	int failedTime = 0;
	    	int totalAllocateTime = 0;
	    	int waitTime = 0;
	        while (status == workingStatus) {
	        	PrintLog();
	        	int needNum = pending.size();
	        	if (needNum <= 0) {
	        		if (waitTime < maxWaitTime)
	        			++waitTime;
	        		else {
	        			LOG.fatal(type + "s can not be started.");
	        			return false;
	        		}
	        		
	        		Thread.sleep(1000);
	        		LOG.info("Wait all the " + type + "s to be started");
	        		continue;
	        	}
	        	else
	        		waitTime = 0;
	        	
	        	LOG.warn("needContainerNum=" + needNum + ", timeElapsed="
	        			+ totalAllocateTime + "\n");
	        	if (totalAllocateTime > allocateTime) {
	            // If allocate failed more than allcateTimeout times, 
	            // release all the Containers, sleep 10 minitus, and then retry.
	            if (failedTime >= maxRetryTime) {
	              LOG.fatal("Allocation failed for " + maxRetryTime + 
	                " times, exiting...");
	              return false;
	            } else {
	              LOG.warn("Allocation timeout of " + allocateTime 
	                 + " seconds reached, "
	                 + " will retry after 10 minitus...");
	              }
	              ++failedTime;
	              totalAllocateTime = 0;
	              // sleep 10 minitues
	              Thread.sleep(10 * 60 * 1000);
	              continue;
	            }
	          
	            int askCount = needNum - requestingContainersNum.get();
	            if (askCount < 0) askCount = 0; 
	            requestingContainersNum.getAndAdd(askCount);
	            LOG.info("Request " + needNum + " containers to run " + type);

	            //need to allocate new machine
	            try {
	                 for (int i = 0; i < askCount; ++i) {
	                     //ContainerRequest containerAsk = setupContainerAskForRM(hosts, racks);
	                   ContainerRequest containerAsk = setupContainerAskForRM(null, null);
	                     amRMClient_.addContainerRequest(containerAsk);
	                 }
	            } catch(Exception e){
	            	e.printStackTrace();;
	            }
	         
	        	Thread.sleep(1000);
	          	totalAllocateTime++;
	        }
	      
	  	    return true;
	      } catch (Exception e){
	        e.printStackTrace();
	        return false;
	      }
	}
	
  /**
   * Setup the request that will be sent to the RM for the container ask.
   * 
   * @return the setup ResourceRequest to be sent to RM
   */
  private ContainerRequest setupContainerAskForRM(String[] nodes, String[] racks) {
    // setup requirements for hosts
    // using * as any host will do for the distributed shell app
    // set the priority for the request
    Priority pri = Records.newRecord(Priority.class);
    // TODO - what is the range for priority? how to decide?
    pri.setPriority(1);

    // Set up resource type requirements
    // For now, memory and CPU are supported so we set memory and cpu
    // requirements
    Resource capability = Records.newRecord(Resource.class);
    capability.setMemory(processMemory_);
    capability.setVirtualCores(processCores_);

    //ContainerRequest request = new ContainerRequest(capability, nodes, racks, pri, false);
    ContainerRequest request = new ContainerRequest(capability, null, null, pri);

     //LOG.info("Requested container ask: " + request.toString());
    return request;
  }
	  
	private void TranferStatus(MyContainer myContainer, MyContainer.MyContainerStatus from, MyContainer.MyContainerStatus to) {
		if (from.equals(to))
			return;
		
		// put the failed container to pendingWorker or pendingServer
		if (!from.equals(MyContainer.MyContainerStatus.Failed)
				&& to.equals(MyContainer.MyContainerStatus.Failed)) {
			Container container = myContainer.container;
			allocatedContainers_.remove(container);
			allocatedContainerIds_.remove(container.getId());
			if (myContainer.type == DSConstants.WORKER)
				pendingWorker.add(myContainer);
			else
				pendingServer.add(myContainer);
		}
		
		// count the number of startedContainers
		if(from.equals(MyContainer.MyContainerStatus.Starting)
				&& (to.equals(MyContainer.MyContainerStatus.Running)
				|| to.equals(MyContainer.MyContainerStatus.Succeed))) {
			if (myContainer.type.equals(DSConstants.WORKER)) {
				++startedWorkerNum;
			} else {
				++startedServerNum;
			}
		}
		
		// count the number of successful workers
		if (to.equals(MyContainer.MyContainerStatus.Succeed)
				&& myContainer.type.equals(DSConstants.WORKER)) {
			++succeedWorkerNum;
		}
		
		// tranfer the status of ContainersManager
		if (status == Status.StartingServer) {
			if (startedServerNum == numServers) {
				GenerateServersList();
				requestingContainersNum.set(0);
				status = Status.StartingWorker; // change to StartingWorker
			}
		} else if (status == Status.StartingWorker) {
			if (to.equals(MyContainer.MyContainerStatus.Failed)
					&& myContainer.type.equals(DSConstants.SERVER)) {
				status = Status.Failed;
			} else if (startedWorkerNum + 1 == numWorkers) {
				status = Status.Running; // change to Running
			}
		} else if (status == Status.Running) {
			if (to.equals(MyContainer.MyContainerStatus.Failed)) {
				status = Status.Failed;
			}
			else if (succeedWorkerNum == numWorkers) {
				status = Status.Succeed;
			}
		}
	}
	
	public void ReportContainerStatus(ContainerId containerId, MyContainer.MyContainerStatus status) {
		synchronized(this) {
			LOG.info("ReportContainerStatus: containerId=" + containerId
				+ ",status=" + status);
			MyContainer myContainer = allocatedContainerIds_.get(containerId);
			if (myContainer == null) {
				LOG.error("Report non-existing container " + containerId);
			}
			else {
				Container container = myContainer.container;
				ReportContainerStatus(container, status);
			}
		}
	}
	
	public void ReportContainerStatus(Container container, MyContainer.MyContainerStatus status) {
		synchronized(this) {
			LOG.info("ReportContainerStatus: containerId=" + container.getId()
				+ ",status=" + status);
			MyContainer myContainer = allocatedContainers_.get(container);
			if (myContainer == null)
				LOG.error("Report non-existing container " + container.getId());
			else {
				MyContainer.MyContainerStatus from = myContainer.status_;
				myContainer.UpdateStatus(status);
				TranferStatus(myContainer, from, status);
			}
		}
	}
	
	public void PutContainers(List<Container> containers) {
		synchronized(this) {
			LOG.info("Get " + containers.size() + " containers");
			requestingContainersNum.getAndAdd(-containers.size());
			while (pendingServer.size() > 0 && containers.size() > 0
					&& status == Status.StartingServer) {
				MyContainer myContainer = pendingServer.get(0);
				Container container = containers.get(0);
				pendingServer.remove(0);
				containers.remove(0);
				allocatedContainers_.put(container, myContainer);
				allocatedContainerIds_.put(container.getId(), myContainer);
				myContainer.Start(container);
			}
			
			while (pendingWorker.size() > 0 && containers.size() > 0
					&& status == Status.StartingWorker) {
				MyContainer myContainer = pendingWorker.get(0);
				Container container = containers.get(0);
				pendingWorker.remove(0);
				containers.remove(0);
				allocatedContainers_.put(container, myContainer);
				allocatedContainerIds_.put(container.getId(), myContainer);
				myContainer.Start(container);
			}
			
			 for (Container container : containers) {
			      LOG.info("Releasing container " + container.getId()
			          + ", containerNode=" + container.getNodeId().getHost()
			          + ":" + container.getNodeId().getPort());
			      amRMClient_.releaseAssignedContainer(container.getId());
			  }
		}
	}
	
	public double GetProgress() {
		return succeedWorkerNum / (0.0 + numWorkers);
	}
	
	public void Stop() {
		status = Status.Failed;
	}
	
    // generate the endpointList file and machinelist file to workerDir
    private void GenerateServersList() {
      try {
      	LOG.info("Generating server list...\n");
        BufferedWriter endpointlist_out = new BufferedWriter(new FileWriter(
            DSConstants.WORKERDIR + File.separator + DSConstants.ENDPOINTLIST));
        BufferedWriter machinelist_out = new BufferedWriter(new FileWriter(
                DSConstants.WORKERDIR + File.separator + DSConstants.MACHINELISTFILE));
        for (MyContainer c : serverBackup) {
        	InetAddress address = InetAddress.getByName(c.container.getNodeId().getHost()); 
        	String ip = address.getHostAddress();
        	endpointlist_out.write(c.id_ + " " + ip + ":" + workerServerPort_ + "\n");
        	machinelist_out.write(ip + "\n");
        }
        endpointlist_out.close();
        machinelist_out.close();
      } catch (Exception e) {
        LOG.error("Failed to generate serverslist " + e + "\n");
      }
    }
};
