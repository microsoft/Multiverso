/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
import java.util.Set;
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
import org.apache.hadoop.yarn.api.records.FinalApplicationStatus;
import org.apache.hadoop.yarn.api.records.Priority;
import org.apache.hadoop.yarn.api.records.Resource;
import org.apache.hadoop.yarn.dmtk.DSConstants;
import org.apache.hadoop.yarn.dmtk.am.CommandFileGenerator;
import org.apache.hadoop.yarn.dmtk.am.ContainersManager.Status;
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

import com.google.common.annotations.VisibleForTesting;

@InterfaceAudience.Public
@InterfaceStability.Unstable
public class ApplicationMaster {
  private static final Log LOG = LogFactory.getLog(ApplicationMaster.class);

  @VisibleForTesting
  @Private
  public static enum AMEvent {
    APP_ATTEMPT_START, APP_ATTEMPT_END, CONTAINER_START, CONTAINER_END
  }

  @VisibleForTesting
  @Private
  public static enum AMEntity {
    APP_ATTEMPT, CONTAINER
  }

  // Configuration
  private Configuration conf;

  // Handle to communicate with the Resource Manager
  @SuppressWarnings("rawtypes")
  private AMRMClientAsync amRMClient;

  // In both secure and non-secure modes, this points to the job-submitter.
  private UserGroupInformation appSubmitterUgi;

  // Handle to communicate with the Node Manager
  private NMClientAsync nmClientAsync;
  // Listen to process the response from the Node Manager
  private NMCallbackHandler containerListener;

  // Application Attempt Id ( combination of attemptId and fail count )
  @VisibleForTesting
  protected ApplicationAttemptId appAttemptID;

  @VisibleForTesting
  private AtomicInteger numRequestingContainers = new AtomicInteger();

  private ContainersManager containersManager;
  private AtomicBoolean isWorkDone = new AtomicBoolean(false);
  FileMonitorThread fileMonitorThread;
  
  // Launch threads
  private ExecutorService containerLauncher = Executors.newFixedThreadPool(30);

  //tokens
  private ByteBuffer allTokens;
 
  // Timeline Client
  private TimelineClient timelineClient;

  // msmpi related
  private String clientAddr = "";
  private int amClientPort = 0;
  private int workerServerPort = 0;
  private int numProcesses = 0;
  private int maxProcessesToRequest = 0;
  private int processMemory = 0;
  private int processCores = 0;
  private int numWorkers = 0;
  private int numServers = 0;
  private boolean isSyncOn = false;
  private boolean isVerboseOn = false;
  private String hdfsAppDir = "";
  private String workerArgs = "";
  private String serverArgs = "";
  
  private static AtomicInteger masterRetVal = new AtomicInteger(-1);
  private Timer timer = new Timer();
  private int allocateTimeout = 0;
  private int executeTimeout = 0;
  
  private CommandFileGenerator commandFileGenerator;
  private YarnClient yarnClient;
  /**
   * @param args
   *          Command line args
   */
  public static void main(String[] args) {
    boolean result = false;
    try {
      ApplicationMaster appMaster = new ApplicationMaster();
      LOG.info("Initializing ApplicationMaster");
      boolean doRun = appMaster.init(args);
      if (!doRun) {
        System.exit(0);
      }
      appMaster.run();
      result = appMaster.finish();
    } catch (Exception e) {
      e.printStackTrace();
      ExitUtil.terminate(1, e);
    }
    if (result) {
      LOG.warn("Application Master completed successfully. exiting");
      System.exit(0);
    } else {
      LOG.warn("Application Master failed. exiting");
      System.exit(2);
    }
  }

  public ApplicationMaster() {
    conf = new YarnConfiguration();
    yarnClient = YarnClient.createYarnClient();
    yarnClient.init(conf);
  }

  /**
   * Parse command line options
   * 
   * @param args
   *          Command line args
   * @return Whether init successful and run should be invoked
   * @throws ParseException
   * @throws IOException
   */
  public boolean init(String[] args) throws ParseException, IOException {
    Map<String, String> envs = System.getenv();
    if (envs.containsKey(DSConstants.ENV_SYNCHRONOUS)) {
      String str = envs.get(DSConstants.ENV_SYNCHRONOUS);
      isSyncOn = Boolean.parseBoolean(str);
    }
    
    if (envs.containsKey(DSConstants.ENV_VERBOSE)) {
      isVerboseOn = Boolean.parseBoolean(envs.get(DSConstants.ENV_VERBOSE));
    }


    //Logger.getRootLogger().addAppender(new ConsoleAppender(null, "System.out"));
    ConsoleAppender console = new ConsoleAppender(); //create appender
    //configure the appender
    //String PATTERN = "%d [%p|%c|%C{1}] %m%n";
    String PATTERN = " [AM] %d %m%n";
    console.setLayout(new PatternLayout(PATTERN));
    if (isVerboseOn)
      console.setThreshold(Level.INFO);
    else
      console.setThreshold(Level.WARN);
    console.setTarget("System.out");
    console.activateOptions();
    //add appender to any Logger (here is root)
    Logger.getRootLogger().addAppender(console);
    
   
    if (isSyncOn) {
      clientAddr = envs.get(DSConstants.ENV_CLIENTADDR);
      amClientPort = Integer.parseInt(envs.get(DSConstants.ENV_AMCLIENTPORT));
      if (amClientPort < 1024 || amClientPort > 65535) {
        throw new IllegalArgumentException(
            "Illegal value passed to amClientPort:" + amClientPort);
      }
    }

    workerArgs = envs.get(DSConstants.ENV_WORKERARGS);
    serverArgs = envs.get(DSConstants.ENV_SERVERARGS);
    workerServerPort = Integer.parseInt(envs.get(DSConstants.ENV_WORKERSERVERPORT));
    processMemory = Integer.parseInt(envs.get(DSConstants.ENV_PROCESSMEMORY));
    processCores = Integer.parseInt(envs.get(DSConstants.ENV_PROCESSCORES));
    numWorkers = Integer.parseInt(envs.get(DSConstants.ENV_NUMWORKERS));
    numServers = Integer.parseInt(envs.get(DSConstants.ENV_NUMSERVERS));
    numProcesses = numWorkers + numServers;

    if (envs.containsKey(DSConstants.ENV_ALLOCATE_TIMEOUT)) {
      allocateTimeout = Integer.parseInt(envs.get(DSConstants.ENV_ALLOCATE_TIMEOUT));
    }
    else {
      // compatible with old client
      allocateTimeout = Integer.MAX_VALUE;
    }
    if (envs.containsKey(DSConstants.ENV_EXECUTE_TIMEOUT)) {
      executeTimeout = Integer.parseInt(envs.get(DSConstants.ENV_EXECUTE_TIMEOUT));
    }
    else {
      executeTimeout = Integer.MAX_VALUE;
    }

    maxProcessesToRequest = numProcesses + 1;

    if (!envs.containsKey(Environment.CONTAINER_ID.name())) {
      throw new IllegalArgumentException(
          "Application Attempt Id not set in the environment");
    } else {
      ContainerId containerId = ConverterUtils.toContainerId(envs
          .get(Environment.CONTAINER_ID.name()));
      appAttemptID = containerId.getApplicationAttemptId();
    }

    if (!envs.containsKey(ApplicationConstants.APP_SUBMIT_TIME_ENV)) {
      throw new RuntimeException(ApplicationConstants.APP_SUBMIT_TIME_ENV
          + " not set in the environment");
    }
    if (!envs.containsKey(Environment.NM_HOST.name())) {
      throw new RuntimeException(Environment.NM_HOST.name()
          + " not set in the environment");
    }
    if (!envs.containsKey(Environment.NM_HTTP_PORT.name())) {
      throw new RuntimeException(Environment.NM_HTTP_PORT
          + " not set in the environment");
    }
    if (!envs.containsKey(Environment.NM_PORT.name())) {
      throw new RuntimeException(Environment.NM_PORT.name()
          + " not set in the environment");
    }

    String sys_platform = "linux";
    if (DSConstants.isWindow == true)
    {
        sys_platform = "window";
    }

    LOG.info("Application master for app running on " + sys_platform + ", appId="
        + appAttemptID.getApplicationId().getId() + ", clustertimestamp="
        + appAttemptID.getApplicationId().getClusterTimestamp()
        + ", attemptId=" + appAttemptID.getAttemptId());

    if (envs.containsKey(DSConstants.ENV_HDFS_APP_DIR)) {
      hdfsAppDir = envs.get(DSConstants.ENV_HDFS_APP_DIR);
    }
    
    commandFileGenerator = new CommandFileGenerator(isVerboseOn, workerServerPort,
      numWorkers, numServers, workerArgs, serverArgs,
      String.valueOf(appAttemptID.getApplicationId().getId()));

    // Creating the Timeline Client
    timelineClient = TimelineClient.createTimelineClient();
    timelineClient.init(conf);
    timelineClient.start();

    return true;
  }

  class FileMonitorThread extends Thread {
    public void run() {
      LOG.info("LOG_DIR: " + System.getenv("LOG_DIRS"));
      FileMonitor fileMonitor = new FileMonitor(isWorkDone);
      fileMonitor.watchFile(System.getenv("LOG_DIRS")
          + File.separator + "AppMaster.stdout", clientAddr, amClientPort);
    }
  }

  /**
   * Main run function for the application master
   * 
   * @throws YarnException
   * @throws IOException
   */
  @SuppressWarnings({"unchecked"})
  public void run() throws YarnException, IOException {
    LOG.warn("ApplicationMaster started");
    /*
    try {
      publishApplicationAttemptEvent(timelineClient, appAttemptID.toString(),
          AMEvent.APP_ATTEMPT_START);
    } catch (Exception e) {
      LOG.error("App Attempt start event coud not be pulished for "
          + appAttemptID.toString(), e);
    }*/
    yarnClient.start();
    if (isSyncOn) {
      fileMonitorThread = new FileMonitorThread();
      fileMonitorThread.start();
    }

    Credentials credentials = UserGroupInformation.getCurrentUser()
        .getCredentials();
    DataOutputBuffer dob = new DataOutputBuffer();
    credentials.writeTokenStorageToStream(dob);
    // Now remove the AM->RM token so that containers cannot access it.
    Iterator<Token<?>> iter = credentials.getAllTokens().iterator();
    while (iter.hasNext()) {
      Token<?> token = iter.next();
      if (token.getKind().equals(AMRMTokenIdentifier.KIND_NAME)) {
        iter.remove();
      }
    }
    allTokens = ByteBuffer.wrap(dob.getData(), 0, dob.getLength());

    // Create appSubmitterUgi and add original tokens to it
    String appSubmitterUserName = System
        .getenv(ApplicationConstants.Environment.USER.name());
    appSubmitterUgi = UserGroupInformation
        .createRemoteUser(appSubmitterUserName);
    appSubmitterUgi.addCredentials(credentials);

    containersManager = new ContainersManager();
    RMCallbackHandler allocListener = new RMCallbackHandler();
    amRMClient = AMRMClientAsync.createAMRMClientAsync(1000, allocListener);
    containerListener = new NMCallbackHandler();
    nmClientAsync = new NMClientAsyncImpl(containerListener);

    containerListener.Init(containersManager, timer, nmClientAsync);
    amRMClient.init(conf);
    amRMClient.start();
    nmClientAsync.init(conf);
    allocListener.Init(containersManager, amRMClient);
    containersManager.Init(numServers, numWorkers, workerServerPort,
        processMemory, processCores, 
        commandFileGenerator, containerListener, allTokens, conf,
        nmClientAsync, amRMClient, containerLauncher);

   
    nmClientAsync.start();

    // Setup local RPC Server to accept status requests directly from
    // clients
    // TODO need to setup a protocol for client to be able to communicate to
    // the RPC server
    // TODO use the rpc port info to register with the RM for the client to
    // send requests to this app master

    // Register self with ResourceManager
    // This will start heartbeating to the RM
    String appMasterHostname = NetUtils.getHostname();
    int appMasterRpcPort = -1;
    String appMasterTrackingUrl = "";
    RegisterApplicationMasterResponse response = amRMClient
        .registerApplicationMaster(appMasterHostname, appMasterRpcPort,
            appMasterTrackingUrl);

	LOG.info("Starting ContainersManager ...");
	containersManager.Start(allocateTimeout, 5, 300);
	try {
	    while (containersManager.status == Status.Running) {
	      Thread.sleep(1000);
	    }
	
	    if (containersManager.status == Status.Succeed) {
	    	masterRetVal.set(0);
	    } else {
	    	masterRetVal.set(-3);
	    }
	    
	    Thread.sleep(10*1000);
	    isWorkDone.set(true);
		} catch(Exception e) {
			e.printStackTrace();
		}
  }

  @VisibleForTesting
  protected boolean finish() throws InterruptedException, YarnException, IOException {
    int executeTime = 0;
    // wait for completion.
    while (!isWorkDone.get()) {
      try {
        Thread.sleep(1000);
        executeTime ++;
        LOG.info("running am elapsed time = " + executeTime);
        if (executeTime > executeTimeout) {
          LOG.fatal("execute timeout of " + executeTimeout 
              + " seconds reached, exiting...");
          masterRetVal.set(-2);
          break;
        }
      } catch (InterruptedException ex) {
        ex.printStackTrace();
      }
    }

    LOG.warn("dmtk master exited with code " + masterRetVal.get());
    synchronized (containerLauncher) {
      containerLauncher.shutdown();
    }

    // When the application completes, it should stop all running containers
    LOG.info("Application completed. Stopping running containers");
    timer.cancel();
    timer.purge();
    

    // When the application completes, it should send a finish application
    // signal to the RM
    LOG.info("Signalling finish to RM");

    FinalApplicationStatus appStatus;
    String appMessage = null;
    boolean success = true;
    if (masterRetVal.get() == 0) {
      appStatus = FinalApplicationStatus.SUCCEEDED;
    } else {
      appStatus = FinalApplicationStatus.FAILED;
      appMessage = "masterRet=" + masterRetVal.get();
      success = false;
    }
    amRMClient.unregisterApplicationMaster(appStatus, appMessage, null);
    //amRMClient.stop();
    synchronized(amRMClient) {
      amRMClient.waitForServiceToStop(10000);
    }
    
    // wait for log shipping to finish
    if (isSyncOn) {
      synchronized(fileMonitorThread)
      {
        fileMonitorThread.join(10000);
      }
    }
    nmClientAsync.waitForServiceToStop(10000);    
    return success;
  }
}
