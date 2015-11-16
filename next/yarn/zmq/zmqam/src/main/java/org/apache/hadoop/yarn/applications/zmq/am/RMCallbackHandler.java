package org.apache.hadoop.yarn.applications.zmq.am;

import java.util.List;
import java.util.concurrent.ConcurrentSkipListSet;
import java.util.concurrent.atomic.AtomicInteger;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.yarn.api.records.Container;
import org.apache.hadoop.yarn.api.records.ContainerExitStatus;
import org.apache.hadoop.yarn.api.records.ContainerId;
import org.apache.hadoop.yarn.api.records.ContainerState;
import org.apache.hadoop.yarn.api.records.ContainerStatus;
import org.apache.hadoop.yarn.api.records.NodeId;
import org.apache.hadoop.yarn.api.records.NodeReport;
import org.apache.hadoop.yarn.applications.zmq.DSConstants;
import org.apache.hadoop.yarn.client.api.async.AMRMClientAsync;

public class RMCallbackHandler implements AMRMClientAsync.CallbackHandler {
	private static final Log LOG = LogFactory.getLog(RMCallbackHandler.class);
	private ContainersManager containersManager_;
	private AMRMClientAsync amRMClient_;
	
	public RMCallbackHandler() {
		
	}
	
	public void Init(ContainersManager containersManager,
			AMRMClientAsync amRMClient) {
		containersManager_ = containersManager;
		amRMClient_ = amRMClient;
	}
	
	@Override
	public void onContainersCompleted(List<ContainerStatus> containers) {
	  for (ContainerStatus containerStatus : containers) {
	      LOG.info("CompletedContainer: " + " containerID="
	          + containerStatus.getContainerId() + ", exitStatus="
	          + containerStatus.getExitStatus() + ", diagnostics: "
	          + containerStatus.getDiagnostics());
	    ContainerId containerId = containerStatus.getContainerId();
	    if (containerStatus.getExitStatus() == ContainerExitStatus.SUCCESS)
	    	containersManager_.ReportContainerStatus(containerId,
	    			MyContainer.MyContainerStatus.Succeed);
	    else
	    	containersManager_.ReportContainerStatus(containerId,
	    			MyContainer.MyContainerStatus.Failed);
	  }
	}
	
	@Override
	public void onContainersAllocated(List<Container> containers) {
		LOG.info("Number of allocated containers: " + containers.size());
		containersManager_.PutContainers(containers);
	}
	
	@Override
	public void onShutdownRequest() {
		containersManager_.Stop();
	}
	
	@Override
	public void onNodesUpdated(List<NodeReport> updatedNodes) {
	}
	
	@Override
	public float getProgress() {
	  // set progress to deliver to RM on next heartbeat
	     return (float)containersManager_.GetProgress();
	}
	
	@Override
    public void onError(Throwable e) {
		containersManager_.Stop();
		amRMClient_.stop();
    }
}
