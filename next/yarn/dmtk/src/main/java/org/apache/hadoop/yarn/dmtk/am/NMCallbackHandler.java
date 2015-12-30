package org.apache.hadoop.yarn.dmtk.am;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.File;
import java.io.IOException;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.lang.System;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import java.util.TreeMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ConcurrentSkipListSet;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.cli.ParseException;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.classification.InterfaceAudience;
import org.apache.hadoop.classification.InterfaceAudience.Private;
import org.apache.hadoop.classification.InterfaceStability;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.DataOutputBuffer;
import org.apache.hadoop.net.NetUtils;
import org.apache.hadoop.security.Credentials;
import org.apache.hadoop.security.UserGroupInformation;
import org.apache.hadoop.security.token.Token;
import org.apache.hadoop.util.ExitUtil;
import org.apache.hadoop.yarn.api.ApplicationConstants;
import org.apache.hadoop.yarn.api.ApplicationConstants.Environment;
import org.apache.hadoop.yarn.api.ContainerManagementProtocol;
import org.apache.hadoop.yarn.api.protocolrecords.RegisterApplicationMasterResponse;
import org.apache.hadoop.yarn.api.records.ApplicationAttemptId;
import org.apache.hadoop.yarn.api.records.Container;
import org.apache.hadoop.yarn.api.records.ContainerId;
import org.apache.hadoop.yarn.api.records.ContainerLaunchContext;
import org.apache.hadoop.yarn.api.records.ContainerState;
import org.apache.hadoop.yarn.api.records.ContainerStatus;
import org.apache.hadoop.yarn.api.records.FinalApplicationStatus;
import org.apache.hadoop.yarn.api.records.LocalResource;
import org.apache.hadoop.yarn.api.records.LocalResourceType;
import org.apache.hadoop.yarn.api.records.LocalResourceVisibility;
import org.apache.hadoop.yarn.api.records.NodeId;
import org.apache.hadoop.yarn.api.records.NodeReport;
import org.apache.hadoop.yarn.api.records.Priority;
import org.apache.hadoop.yarn.api.records.Resource;
import org.apache.hadoop.yarn.dmtk.DSConstants;
import org.apache.hadoop.yarn.dmtk.am.CommandFileGenerator;
import org.apache.hadoop.yarn.api.records.ContainerExitStatus;
import org.apache.hadoop.yarn.client.api.AMRMClient.ContainerRequest;
import org.apache.hadoop.yarn.client.api.TimelineClient;
import org.apache.hadoop.yarn.client.api.YarnClient;
import org.apache.hadoop.yarn.client.api.async.AMRMClientAsync;
import org.apache.hadoop.yarn.client.api.async.NMClientAsync;
import org.apache.hadoop.yarn.client.api.async.impl.NMClientAsyncImpl;
import org.apache.hadoop.yarn.conf.YarnConfiguration;
import org.apache.hadoop.yarn.exceptions.YarnException;
import org.apache.hadoop.yarn.security.AMRMTokenIdentifier;
import org.apache.hadoop.yarn.util.ConverterUtils;
import org.apache.hadoop.yarn.util.Records;
import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.apache.log4j.ConsoleAppender;
import org.apache.log4j.PatternLayout;

public class NMCallbackHandler implements NMClientAsync.CallbackHandler {
	private static final long statusUpdateInterval = 2000;
	private static final Log LOG = LogFactory.getLog(NMCallbackHandler.class);
	private ContainersManager containersManager_;
	private NMClientAsync nmClientAsync_;
	private Timer timer_;
  private int containerStatusReceivedNum_;
	private ConcurrentMap<ContainerId, Container> containers = new ConcurrentHashMap<ContainerId, Container>();

    public NMCallbackHandler() {}

    public void Init(ContainersManager containersManager,
    		Timer timer, NMClientAsync nmClientAsync) {
    	containersManager_ = containersManager;
    	timer_ = timer;
    	nmClientAsync_ = nmClientAsync;
      containerStatusReceivedNum_ = 0;
    }

    public void addContainer(ContainerId containerId, Container container) {
        containers.putIfAbsent(containerId, container);
      }

    
    @Override
    public void onContainerStopped(ContainerId containerId) {
      LOG.info("Succeeded to stop Container " + containerId);
      Container container = containers.get(containerId);
      containersManager_.ReportContainerStatus(container, MyContainer.MyContainerStatus.Failed);
      containers.remove(containerId);
    }

    @Override
    public void onContainerStatusReceived(ContainerId containerId,
        ContainerStatus containerStatus) {
      ++containerStatusReceivedNum_;
      Container container = containers.get(containerId);
      if (container == null) {
        LOG.error("got status report from non-existing container "
            + containerId);
        return;
      }
      
      if (containerStatus.getState() == ContainerState.RUNNING) {
    	containersManager_.ReportContainerStatus(container,
    	    		  MyContainer.MyContainerStatus.Running);
       if (containerStatusReceivedNum_ % 1000 == 0) {
          LOG.info("Query Status of Containers " + containerStatusReceivedNum_ + " times\n");
      }

        scheduleStatusUpdate(container);
      } else if (containerStatus.getState() == ContainerState.COMPLETE) {
        LOG.info("Container Status: id=" + containerId + ", status="
          + containerStatus.getExitStatus());
        if (containerStatus.getExitStatus() == ContainerExitStatus.SUCCESS)
            containersManager_.ReportContainerStatus(container,
          		  MyContainer.MyContainerStatus.Succeed);
        else
        	containersManager_.ReportContainerStatus(container,
            		  MyContainer.MyContainerStatus.Failed);
      } else {
        // containerStatus.getState() == ContainerState.NEW
      }
    }

    @Override
    public void onContainerStarted(ContainerId containerId,
        Map<String, ByteBuffer> allServiceResponse) {
      LOG.info("container started: " + containerId.getContainerId());
      Container container = containers.get(containerId);
      containersManager_.ReportContainerStatus(container, MyContainer.MyContainerStatus.Running);
      scheduleStatusUpdate(container);
    }

    @Override
    public void onStartContainerError(ContainerId containerId, Throwable t) {
      LOG.info("Failed to start Container " + containerId + " on " 
          + containers.get(containerId).getNodeId().getHost()
          + " : " + t.getMessage());
      Container container = containers.get(containerId);
      containers.remove(containerId);
      containersManager_.ReportContainerStatus(container, MyContainer.MyContainerStatus.Failed);
    }

    @Override
    public void onGetContainerStatusError(ContainerId containerId, Throwable t) {
      LOG.info("Failed to query the status of Container " + containerId);
      if (containers.containsKey(containerId)) {
        scheduleStatusUpdate(containers.get(containerId));
      }
    }

    @Override
    public void onStopContainerError(ContainerId containerId, Throwable t) {
      LOG.info("Failed to stop Container " + containerId);
    }

    private void scheduleStatusUpdate(Container c) {
      GetContainerStatusTask task = new GetContainerStatusTask(c.getId(),
          c.getNodeId());
      timer_.schedule(task, statusUpdateInterval);
    }

    private class GetContainerStatusTask extends TimerTask {
      public GetContainerStatusTask(ContainerId cid, NodeId nid) {
        cid_ = cid;
        nid_ = nid;
      }

      public void run() {
        try {
          nmClientAsync_.getContainerStatusAsync(cid_, nid_);
        } catch (Exception e) {
          System.out.println("error running thread " + e.getMessage());
        }
      }

      private ContainerId cid_;
      private NodeId nid_;
    }

  }
