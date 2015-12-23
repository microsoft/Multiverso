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

package org.apache.hadoop.yarn.dmtk;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Vector;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.io.IOUtils;
import org.apache.hadoop.classification.InterfaceAudience;
import org.apache.hadoop.classification.InterfaceStability;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.permission.FsPermission;
import org.apache.hadoop.io.DataOutputBuffer;
import org.apache.hadoop.security.Credentials;
import org.apache.hadoop.security.UserGroupInformation;
import org.apache.hadoop.security.token.Token;
import org.apache.hadoop.yarn.api.ApplicationClientProtocol;
import org.apache.hadoop.yarn.api.ApplicationConstants;
import org.apache.hadoop.yarn.api.ApplicationConstants.Environment;
import org.apache.hadoop.yarn.api.protocolrecords.GetNewApplicationResponse;
import org.apache.hadoop.yarn.api.protocolrecords.KillApplicationRequest;
import org.apache.hadoop.yarn.api.records.ApplicationId;
import org.apache.hadoop.yarn.api.records.ApplicationReport;
import org.apache.hadoop.yarn.api.records.ApplicationSubmissionContext;
import org.apache.hadoop.yarn.api.records.ContainerLaunchContext;
import org.apache.hadoop.yarn.api.records.FinalApplicationStatus;
import org.apache.hadoop.yarn.api.records.LocalResource;
import org.apache.hadoop.yarn.api.records.LocalResourceType;
import org.apache.hadoop.yarn.api.records.LocalResourceVisibility;
import org.apache.hadoop.yarn.api.records.Priority;
import org.apache.hadoop.yarn.api.records.QueueACL;
import org.apache.hadoop.yarn.api.records.QueueInfo;
import org.apache.hadoop.yarn.api.records.QueueUserACLInfo;
import org.apache.hadoop.yarn.api.records.Resource;
import org.apache.hadoop.yarn.api.records.YarnApplicationState;
import org.apache.hadoop.yarn.api.records.YarnClusterMetrics;
import org.apache.hadoop.yarn.dmtk.am.ApplicationMaster;
import org.apache.hadoop.yarn.client.api.YarnClient;
import org.apache.hadoop.yarn.client.api.YarnClientApplication;
import org.apache.hadoop.yarn.conf.YarnConfiguration;
import org.apache.hadoop.yarn.exceptions.YarnException;
import org.apache.hadoop.yarn.util.ConverterUtils;
import org.apache.hadoop.yarn.util.Records;

import java.net.InetAddress;
import java.io.*;
import java.net.*;

import org.apache.log4j.Level;
import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.apache.log4j.ConsoleAppender;
import org.apache.log4j.PatternLayout;
/**
 * Client for Distributed Shell application submission to YARN.
 * 
 * <p>
 * The distributed shell client allows an application master to be launched that
 * in turn would run the provided shell command on a set of containers.
 * </p>
 * 
 * <p>
 * This client is meant to act as an example on how to write yarn-based
 * applications.
 * </p>
 * 
 * <p>
 * To submit an application, a client first needs to connect to the
 * <code>ResourceManager</code> aka ApplicationsManager or ASM via the
 * {@link ApplicationClientProtocol}. The {@link ApplicationClientProtocol}
 * provides a way for the client to get access to cluster information and to
 * request for a new {@link ApplicationId}.
 * <p>
 * 
 * <p>
 * For the actual job submission, the client first has to create an
 * {@link ApplicationSubmissionContext}. The
 * {@link ApplicationSubmissionContext} defines the application details such as
 * {@link ApplicationId} and application name, the priority assigned to the
 * application and the queue to which this application needs to be assigned. In
 * addition to this, the {@link ApplicationSubmissionContext} also defines the
 * {@link ContainerLaunchContext} which describes the <code>Container</code>
 * with which the {@link ApplicationMaster} is launched.
 * </p>
 * 
 * <p>
 * The {@link ContainerLaunchContext} in this scenario defines the resources to
 * be allocated for the {@link ApplicationMaster}'s container, the local
 * resources (jars, configuration files) to be made available and the
 * environment to be set for the {@link ApplicationMaster} and the commands to
 * be executed to run the {@link ApplicationMaster}.
 * <p>
 * 
 * <p>
 * Using the {@link ApplicationSubmissionContext}, the client submits the
 * application to the <code>ResourceManager</code> and then monitors the
 * application by requesting the <code>ResourceManager</code> for an
 * {@link ApplicationReport} at regular time intervals. In case of the
 * application taking too long, the client kills the application by submitting a
 * {@link KillApplicationRequest} to the <code>ResourceManager</code>.
 * </p>
 * 
 */
@InterfaceAudience.Public
@InterfaceStability.Unstable
public class Client {

  private static final Logger LOG = Logger.getLogger(Client.class);

  // Configuration
  private Configuration conf;
  private YarnClient yarnClient;

  // msmpi specific info
  private static boolean isSyncOn = false;
  private static boolean isVerboseOn = false;

  private int amClientPort = 0;
  private int workerServerPort = 0;

  private int processMemory = 0;
  private int processCores = 0;
  private int numWorkers = 0;
  private int numServers = 0;
  private int allocateTimeout = 0;
  private int executeTimeout = 0;
  private String amJarPath = "";
  private String workerArgs = "";
  private String serverArgs = "";

  // Application master specific info to register a new Application with
  // RM/ASM
  private String appName = "dmtk";
  // App master priority
  private int amPriority = 0;
  // Queue for App master
  private String amQueue = "default";
  // Main class to invoke application master
  private String appMasterMainClass = "";

  // log4j.properties file
  // if available, add to local resources and set into classpath
  private String log4jPropFile = "";
  // Hardcoded path to custom log_properties
  private static final String log4jPath = "log4j.properties";

  // flag to indicate whether to keep containers across application attempts.
  private static final boolean keepContainers = false;

  // Debug flag
  boolean debugFlag = false;
  // Command line options
  private Options opts;
  private boolean isKilled = false;

  /**
   * @param args
   *          Command line arguments
   */
  public static void main(String[] args) {
    boolean result = false;
    try {
      Client client = new Client();
      try {
        boolean doRun = client.init(args);
        if (isVerboseOn) {
          LogManager.getRootLogger().setLevel(Level.INFO);
          LOG.info("Initializing Client");
        }
        else {
          LogManager.getRootLogger().setLevel(Level.WARN);
        }
        if (!doRun) {
          System.exit(0);
        }
      } catch (IllegalArgumentException e) {
        System.err.println(e.getLocalizedMessage());
        client.printUsage();
        System.exit(-1);
      }
      result = client.run();
      LOG.info("run() returned " + result);
    } catch (Exception e) {
      LOG.fatal("Error running Client:" + e.getLocalizedMessage());
      e.printStackTrace();
      System.exit(1);
    }
    if (isSyncOn) {
      if (result) {
        LOG.fatal("Application finished successfully.");
        System.exit(0);
      }
      else {
        LOG.fatal("Application failed.");
        System.exit(2);
      }
    }
    else
    {
      if (result) {
        LOG.fatal("Appllication submitted successfully.");
        System.exit(0);
      }
      else {
        LOG.fatal("Application submission failed.");
        System.exit(2);
      }
    }
  }

  /**
   */
  public Client(Configuration conf) throws Exception {
    this("org.apache.hadoop.yarn.dmtk.am.ApplicationMaster", conf);
  }

  Client(String appMasterMainClass, Configuration conf) {
    this.conf = conf;
    this.appMasterMainClass = appMasterMainClass;
    yarnClient = YarnClient.createYarnClient();
    yarnClient.init(conf);

    opts = new Options();
    opts.addOption(DSConstants.OPT_HELP, false, "Print usage");
    opts.addOption(DSConstants.OPT_QUEUE, true, "Job queue");
    opts.addOption(DSConstants.OPT_APPNAME, true, "App name");
    opts.addOption(DSConstants.OPT_WORKERARGS, true,
            "The args for worker/start.bat");
    opts.addOption(DSConstants.OPT_SERVERARGS, true,
            "The args for server/start.bat");    
    opts.addOption(DSConstants.OPT_AMCLIENTPORT, true,
            "The port am should use to communicate with client");
    opts.addOption(DSConstants.OPT_WORKERSERVERPORT, true,
            "The port servers should use to communicate with workers");
    opts.addOption(DSConstants.OPT_PROCESSMEMORY, true,
        "Memory used by each process (in MB). Default 1024");
    opts.addOption(DSConstants.OPT_PROCESSCORES, true,
        "Number of cores used by each process. Default 1");
    opts.addOption(DSConstants.OPT_NUMWORKERS, true,
        "The number of workers. Default 1");
    opts.addOption(DSConstants.OPT_NUMSERVERS, true,
        "The number of servers. Default 1");
    opts.addOption(DSConstants.OPT_DEBUG, false, "Enable debug info");
    opts.addOption(DSConstants.OPT_VERBOSE, false, "Enable verbose output");
    opts.addOption(DSConstants.OPT_SYNCHRONOUS, false,
            "Wait till the program exits");
    opts.addOption(DSConstants.OPT_ALLOCATE_TIMEOUT, true, "Maximum time to wait while allocating containers, default "
            + DSConstants.DEFAULT_ALLOCATE_TIMEOUT + " in seconds");
    opts.addOption(DSConstants.OPT_EXECUTE_TIMEOUT, true, "Maximum time to wait while executing app, default " + 
    		DSConstants.DEFAULT_EXECUTE_TIMEOUT + " in seconds");
    opts.addOption(DSConstants.OPT_JAR_FILE, true, "AM jar file");
  }

  /**
   */
  public Client() throws Exception {
    this(new YarnConfiguration());
  }

  /**
   * Helper function to print out usage
   */
  private void printUsage() {
    new HelpFormatter().printHelp("Client", opts);
  }

  /**
   * Parse command line options
   * 
   * @param args
   *          Parsed command line options
   * @return Whether the init was successful to run the client
   * @throws ParseException
   */
  public boolean init(String[] args) throws ParseException {
    CommandLine cliParser = new GnuParser().parse(opts, args);

    if (args.length == 0) {
      throw new IllegalArgumentException(
          "No args specified for client to initialize");
    }
    
    if (cliParser.hasOption(DSConstants.OPT_HELP)) {
        printUsage();
        return false;
      }
    
    if (cliParser.hasOption(DSConstants.OPT_VERBOSE)) {
        isVerboseOn = true;
    }
        
    ConsoleAppender console = new ConsoleAppender(); //create appender
    String PATTERN = "[Client] %d %m%n";
    console.setLayout(new PatternLayout(PATTERN));
    //configure the appender
    if (isVerboseOn)
      console.setThreshold(Level.INFO);
    else
      console.setThreshold(Level.WARN);
    console.setTarget("System.out");
    console.activateOptions();
    //add appender to any Logger (here is root)
    Logger.getRootLogger().addAppender(console);
    
    if (cliParser.hasOption(DSConstants.OPT_SYNCHRONOUS)) {
        isSyncOn = true;
    }
    
    if (isSyncOn) {
      try {
        amClientPort = Integer.parseInt(cliParser
            .getOptionValue(DSConstants.OPT_AMCLIENTPORT));
      } catch (Exception e) {
        e.printStackTrace();
        throw new IllegalArgumentException(
            "Illegal number passed to clientport");
      }
    }
    
    if (!cliParser.hasOption(DSConstants.OPT_WORKERSERVERPORT)) {
    	throw new IllegalArgumentException("The option " + DSConstants.OPT_WORKERSERVERPORT + " is missing");
    }
    
    try {
      numWorkers = Integer.parseInt(cliParser.getOptionValue(
          DSConstants.OPT_NUMWORKERS, "1"));
       numServers = Integer.parseInt(cliParser.getOptionValue(
          DSConstants.OPT_NUMSERVERS, "1"));
      processMemory = Integer.parseInt(cliParser.getOptionValue(
          DSConstants.OPT_PROCESSMEMORY, "1024"));
      processCores = Integer.parseInt(cliParser.getOptionValue(
          DSConstants.OPT_PROCESSCORES, "1"));
      workerServerPort = Integer.parseInt(cliParser.getOptionValue(
          DSConstants.OPT_WORKERSERVERPORT));
    } catch (Exception e) {
      e.printStackTrace();
      throw new IllegalArgumentException(
          "Illegal number passed to numProcesses, processMemory, processCores, RUNTYPE or WorkerServerPort");
    }
    
    if (workerServerPort < 1024 || workerServerPort > 65535) {
        throw new IllegalArgumentException("Illegal port number: " + workerServerPort);
      }
    
    if (isSyncOn && (amClientPort < 1024 || amClientPort > 65535)) {
      throw new IllegalArgumentException("Illegal port number: " + amClientPort);
    }
    
    if (numWorkers + numServers < 1 || processMemory < 0 || processCores < 1) {
      throw new IllegalArgumentException(
          "Invalid no. of machine or process memory/vcores specified,"
              + " exiting." + " Specified numWorkers=" + numWorkers
              + ",numServers=" + numServers
              + ", processMemory=" + processMemory + ", processCores="
              + processCores);
    }

    workerArgs = cliParser.getOptionValue(DSConstants.OPT_WORKERARGS, "");
    serverArgs = cliParser.getOptionValue(DSConstants.OPT_SERVERARGS, "");    
    amQueue = cliParser.getOptionValue(DSConstants.OPT_QUEUE, "default");
    appName = cliParser.getOptionValue(DSConstants.OPT_APPNAME, "dmtk");
    
    try {
      allocateTimeout = Integer.parseInt(cliParser.getOptionValue(DSConstants.OPT_ALLOCATE_TIMEOUT, "0"));
      executeTimeout = Integer.parseInt(cliParser.getOptionValue(DSConstants.OPT_EXECUTE_TIMEOUT, "0"));
      if (allocateTimeout == 0) {
        allocateTimeout = DSConstants.DEFAULT_ALLOCATE_TIMEOUT;
      }
      else if (allocateTimeout < 0) {
        allocateTimeout = Integer.MAX_VALUE;
      }
      if (executeTimeout == 0) {
        executeTimeout = DSConstants.DEFAULT_EXECUTE_TIMEOUT;
      }
      else if (executeTimeout < 0) {
        executeTimeout = Integer.MAX_VALUE;
      }
    } catch (Exception e) {
      e.printStackTrace();
      throw new IllegalArgumentException("Wrong option for timeout");
    }
    
    if (!cliParser.hasOption(DSConstants.OPT_JAR_FILE)) {
      throw new IllegalArgumentException("You should provide the ApplicationMaster jar");
    }
    amJarPath = cliParser.getOptionValue(DSConstants.OPT_JAR_FILE, "");
    return true;
  }

  /**
   * Main run function for the client
   * 
   * @return true if application completed successfully
   * @throws IOException
   * @throws YarnException
   */
  public boolean run() throws IOException, YarnException {

    if (isVerboseOn) {
      LOG.info("Running Client");
    }
    yarnClient.start();

    if (isVerboseOn) {
      YarnClusterMetrics clusterMetrics = yarnClient.getYarnClusterMetrics();
      LOG.info("Got Cluster metric info from ASM" + ", numNodeManagers="
          + clusterMetrics.getNumNodeManagers());
      
       
      QueueInfo queueInfo = yarnClient.getQueueInfo(this.amQueue);
      LOG.info("Queue info" + ", queueName=" + queueInfo.getQueueName()
          + ", queueCurrentCapacity=" + queueInfo.getCurrentCapacity()
          + ", queueMaxCapacity=" + queueInfo.getMaximumCapacity()
          + ", queueApplicationCount=" + queueInfo.getApplications().size()
          + ", queueChildQueueCount=" + queueInfo.getChildQueues().size());

      List<QueueUserACLInfo> listAclInfo = yarnClient.getQueueAclsInfo();
      for (QueueUserACLInfo aclInfo : listAclInfo) {
        for (QueueACL userAcl : aclInfo.getUserAcls()) {
          LOG.info("User ACL Info for Queue" + ", queueName="
              + aclInfo.getQueueName() + ", userAcl=" + userAcl.name());
        }
      }
    }

    // Get a new application id
    YarnClientApplication app = yarnClient.createApplication();
    GetNewApplicationResponse appResponse = app.getNewApplicationResponse();

    // If we do not have min/max, we may not be able to correctly request
    // the required resources from the RM for the app master
    // Memory ask has to be a multiple of min and less than max.
    // Dump out information about cluster capability as seen by the resource
    // manager
    int maxMem = appResponse.getMaximumResourceCapability().getMemory();
    int maxVCores = appResponse.getMaximumResourceCapability()
        .getVirtualCores();
    if (isVerboseOn) {
      LOG.info("Max mem capabililty of resources in this cluster " + maxMem);
      LOG.info("Max virtual cores capabililty of resources in this cluster "
          + maxVCores);
    }

    // A resource ask cannot exceed the max.
    if (processMemory > maxMem) {
      LOG.info("Process memory specified above max threshold of cluster. Using max value."
          + ", specified=" + processMemory + ", max=" + maxMem);
      processMemory = maxMem;
    }

    if (processCores > maxVCores) {
      LOG.info("Process vcores specified above max threshold of cluster. "
          + "Using max value." + ", specified=" + processCores + ", max="
          + maxVCores);
      processCores = maxVCores;
    }

    // set the application name
    ApplicationSubmissionContext appContext = app
        .getApplicationSubmissionContext();
    ApplicationId appId = appContext.getApplicationId();
    PrintWriter jobIdListFile = new PrintWriter("JOB_ID_LIST.txt");
    jobIdListFile.println(appId.toString());
    jobIdListFile.close();

    appContext.setKeepContainersAcrossApplicationAttempts(keepContainers);
    appContext.setApplicationName(appName);

    // Set up the container launch context for the application master
    ContainerLaunchContext amContainer = Records
        .newRecord(ContainerLaunchContext.class);

    // set local resources for the application master
    // local files or archives as needed
    // In this scenario, the jar file for the application master is part of
    // the local resources
    Map<String, LocalResource> localResources = new HashMap<String, LocalResource>();

    // Copy the application master jar to the filesystem
    // Create a local resource to point to the destination jar path
    FileSystem fs = FileSystem.get(conf);

    // Set the log4j properties if needed
    if (!log4jPropFile.isEmpty()) {
      addToLocalResources(fs, log4jPropFile, log4jPath, appId.getId(),
          localResources, null);
    }
    
    // am jar
    LOG.info("using AM jar from local path: " + amJarPath);
    addToLocalResources(fs, amJarPath,
        DSConstants.JARFILE, appId.getId(), localResources, null);
    
    LOG.info("Add local server folder ...");
    addFolderToLocalResources(fs, new File(DSConstants.LOCALSERVERDIR).getAbsolutePath(),
      DSConstants.SERVERDIR, appId.getId(), localResources, null);
    LOG.info("Add local worker folder ...");
    addFolderToLocalResources(fs, new File(DSConstants.LOCALWORKERDIR).getAbsolutePath(),
      DSConstants.WORKERDIR, appId.getId(), localResources, null);
    
    // Set local resource info into app master container launch context
    amContainer.setLocalResources(localResources);

    // Set the necessary security tokens as needed
    // amContainer.setContainerTokens(containerToken);

    // Set the env variables to be setup in the env where the application
    // master will be run
    if (isVerboseOn) {
      LOG.info("Set the environment for the application master");
    }
    Map<String, String> env = new HashMap<String, String>();
    InetAddress addr = InetAddress.getLocalHost();
    env.put(DSConstants.ENV_CLIENTADDR, addr.getHostName());
    env.put(DSConstants.ENV_AMCLIENTPORT, String.valueOf(amClientPort));
    env.put(DSConstants.ENV_WORKERSERVERPORT, String.valueOf(workerServerPort));    
    env.put(DSConstants.ENV_PROCESSMEMORY, String.valueOf(processMemory));
    env.put(DSConstants.ENV_PROCESSCORES, String.valueOf(processCores));
    env.put(DSConstants.ENV_NUMWORKERS, String.valueOf(numWorkers));
    env.put(DSConstants.ENV_NUMSERVERS, String.valueOf(numServers));
    env.put(DSConstants.ENV_WORKERARGS, workerArgs);
    env.put(DSConstants.ENV_SERVERARGS, serverArgs);
    env.put(DSConstants.ENV_VERBOSE, String.valueOf(isVerboseOn));
    env.put(DSConstants.ENV_SYNCHRONOUS, String.valueOf(isSyncOn));
    env.put(DSConstants.ENV_ALLOCATE_TIMEOUT, String.valueOf(allocateTimeout));
    env.put(DSConstants.ENV_EXECUTE_TIMEOUT, String.valueOf(executeTimeout));

    String homeDir = fs.getHomeDirectory().toUri().getRawPath();
    String appDir = homeDir + "/" + appName + "/" + appId.getId() + "/";
    // Path p = new Path(fs.getHomeDirectory(), appDir);
    env.put(DSConstants.ENV_HDFS_APP_DIR, appDir);

    StringBuilder classPathEnv = new StringBuilder(Environment.CLASSPATH.$$())
        .append(ApplicationConstants.CLASS_PATH_SEPARATOR).append("./*");
    for (String c : conf.getStrings(
        YarnConfiguration.YARN_APPLICATION_CLASSPATH,
        YarnConfiguration.DEFAULT_YARN_CROSS_PLATFORM_APPLICATION_CLASSPATH)) {
      classPathEnv.append(ApplicationConstants.CLASS_PATH_SEPARATOR);
      classPathEnv.append(c.trim());
    }
    classPathEnv.append(ApplicationConstants.CLASS_PATH_SEPARATOR).append(
        "./log4j.properties");

    // add the runtime classpath needed for tests to work
    if (conf.getBoolean(YarnConfiguration.IS_MINI_YARN_CLUSTER, false)) {
      classPathEnv.append(':');
      classPathEnv.append(System.getProperty("java.class.path"));
    }

    env.put("CLASSPATH", classPathEnv.toString());

    amContainer.setEnvironment(env);

    // Set the necessary command to execute the application master
    Vector<CharSequence> vargs = new Vector<CharSequence>(30);

    // Set java executable command
    if (isVerboseOn) {
      LOG.info("Setting up app master command");
    }

    if (DSConstants.isWindow) {
      vargs.add("dir /s \n");
    } else {
      vargs.add("ls  \n");
    }
    
    vargs.add(Environment.JAVA_HOME.$$() + "/bin/java");
    // Set Xmx based on am memory size
    vargs.add("-Xmx4096m");
    // Set class name
    vargs.add(appMasterMainClass);
    vargs.add("1>" + ApplicationConstants.LOG_DIR_EXPANSION_VAR
        + File.separator + "AppMaster.stdout");
    vargs.add("2>" + ApplicationConstants.LOG_DIR_EXPANSION_VAR
        + File.separator + "AppMaster.stderr");

    // Get final commmand
    StringBuilder command = new StringBuilder();
    for (CharSequence str : vargs) {
      command.append(str).append(" ");
    }

    if (isVerboseOn) {
      LOG.info("Completed setting up app master command " + command.toString());
    }
    List<String> commands = new ArrayList<String>();
    commands.add(command.toString());
    amContainer.setCommands(commands);

    // Set up resource type requirements
    // For now, both memory and vcores are supported, so we set memory and
    // vcores requirements
    Resource capability = Records.newRecord(Resource.class);
    capability.setMemory(4096);
    capability.setVirtualCores(4);
    appContext.setResource(capability);

    // Service data is a binary blob that can be passed to the application
    // Not needed in this scenario
    // amContainer.setServiceData(serviceData);

    // Setup security tokens
    if (UserGroupInformation.isSecurityEnabled()) {
      Credentials credentials = new Credentials();
      String tokenRenewer = conf.get(YarnConfiguration.RM_PRINCIPAL);
      if (tokenRenewer == null || tokenRenewer.length() == 0) {
        throw new IOException(
            "Can't get Master Kerberos principal for the RM to use as renewer");
      }

      // For now, only getting tokens for the default file-system.
      final Token<?> tokens[] = fs.addDelegationTokens(tokenRenewer,
          credentials);
      if (tokens != null) {
        for (Token<?> token : tokens) {
          LOG.info("Got dt for " + fs.getUri() + "; " + token);
        }
      }
      DataOutputBuffer dob = new DataOutputBuffer();
      credentials.writeTokenStorageToStream(dob);
      ByteBuffer fsTokens = ByteBuffer.wrap(dob.getData(), 0, dob.getLength());
      amContainer.setTokens(fsTokens);
    }

    appContext.setAMContainerSpec(amContainer);

    // Set the priority for the application master
    Priority pri = Records.newRecord(Priority.class);
    pri.setPriority(amPriority);
    appContext.setPriority(pri);

    // Set the queue to which this application is to be submitted in the RM
    appContext.setQueue(amQueue);

    // Submit the application to the applications manager
    // SubmitApplicationResponse submitResp =
    // applicationsManager.submitApplication(appRequest);
    // Ignore the response as either a valid response object is returned on
    // success
    // or an exception thrown to denote some form of a failure
    if (isVerboseOn) {
      LOG.info("Submitting application to ASM");
    }

    yarnClient.submitApplication(appContext);

    // TODO
    // Try submitting the same request again
    // app submission failure?

    // Monitor the application
    return monitorApplication(appId);
  }

  class AMThread extends Thread {
    public void run() {
      LOG.info("App starts running...");
      try {
        ServerSocket server = null;
        try {
          server = new ServerSocket(amClientPort);
        } catch (Exception e) {
          e.printStackTrace();
          return;
        }
        Socket socket = null;
        while (true) {
          try {
            socket = server.accept();
            LOG.info("am connected to port " + socket.getLocalPort());
          } catch (Exception e) {
            e.printStackTrace();
            server.close();
            return;
          }
          BufferedReader is = new BufferedReader(new InputStreamReader(
              socket.getInputStream()));
          String line = new String();
          while ((line = is.readLine()) != null) {
            if (line.contains(DSConstants.AM_END_OUTPUT)) break;
            System.out.println(line);
          }
          is.close();
          socket.close();
          server.close();
          return;
        }
      } catch (Exception e) {
        LOG.info("AMThread hit an exception: " + e.toString());
      } finally {
        LOG.info("AMThread::run exit");
      }
    }
  }

  class InputThread extends Thread {
    public void run() {
      try {
        BufferedReader sin = new BufferedReader(
            new InputStreamReader(System.in));
        String command = sin.readLine();
        while (!command.equals("kill")) {
          command = sin.readLine();
        }
        if (command.equals("kill")) {
          isKilled = true;
        }
      } catch (Exception e) {
        e.printStackTrace();
      }
    }
  }

  /**
   * Monitor the submitted application for completion. Kill application if time
   * expires.
   * 
   * @param appId
   *          Application Id of application to be monitored
   * @return true if application completed successfully
   * @throws YarnException
   * @throws IOException
   * @throws InterruptedException 
   */
  private boolean monitorApplication(ApplicationId appId) throws YarnException,
      IOException {
    if (!isSyncOn) {
      System.out.println("Application started successfully.\n" + "id = "
          + appId.toString());
      return true;
    }
    else {
      System.out.println("Starting to monitor program stdout.");
    }

    boolean retVal = false;
    AMThread amThread = new AMThread();
    amThread.start();

    InputThread inputThread = new InputThread();
    inputThread.start();

    while (true) {
      // Check app status every 1 second.
      try {
        Thread.sleep(1000);
      } catch (InterruptedException e) {
        LOG.debug("Thread sleep in monitoring loop interrupted");
      }

      // Get application report for the appId we are interested in
      ApplicationReport report = yarnClient.getApplicationReport(appId);

      YarnApplicationState state = report.getYarnApplicationState();
      FinalApplicationStatus dsStatus = report.getFinalApplicationStatus();
      if (YarnApplicationState.FINISHED == state) {
        if (FinalApplicationStatus.SUCCEEDED == dsStatus) {
          LOG.info("Application has completed successfully. Breaking monitoring loop");
          retVal = true;
          break;
        } else {
          LOG.info("Application did finished unsuccessfully." + " YarnState="
              + state.toString() + ", DSFinalStatus=" + dsStatus.toString()
              + ". Breaking monitoring loop");
          retVal = false;
          break;
        }
      } else if (YarnApplicationState.KILLED == state
          || YarnApplicationState.FAILED == state) {
        LOG.info("Application did not finish." + " YarnState="
            + state.toString() + ", DSFinalStatus=" + dsStatus.toString()
            + ". Breaking monitoring loop");
        retVal = false;
        break;
      }

      if (isKilled) {
        LOG.info("Application will be killed...");
        forceKillApplication(appId);
      }
    }
    try {
      synchronized(amThread) {
        LOG.info("Waiting for am thread to exit");
        amThread.join(10000);
        LOG.info("AM monitoring thread exit");
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
    return retVal;
  }

  /**
   * Kill a submitted application by sending a call to the ASM
   * 
   * @param appId
   *          Application Id to be killed.
   * @throws YarnException
   * @throws IOException
   */
  private void forceKillApplication(ApplicationId appId) throws YarnException,
      IOException {
    // Response can be ignored as it is non-null on success or
    // throws an exception in case of failures
    yarnClient.killApplication(appId);
  }

  private void addFolderToLocalResources(FileSystem fs, String fileSrcPath,
      String fileDstPath, int appId, Map<String, LocalResource> localResources,
      String resources) throws IOException {
    File file = new File(fileSrcPath);
    if (file.isDirectory()) {
        File[] files = file.listFiles();
        for (File f : files) {
          String dstFile = fileDstPath + File.separator
              + f.getName();
          addFolderToLocalResources(fs, f.getAbsolutePath(), dstFile,
              appId, localResources, null);
        }
    } else {
      addToLocalResources(fs, file.getAbsolutePath(), fileDstPath, appId,
        localResources, null);
      LOG.info("Add local file " + file.getName());
    }
  }

  private void addToLocalResources(FileSystem fs, String fileSrcPath,
      String fileDstPath, int appId, Map<String, LocalResource> localResources,
      String resources) throws IOException {
    String suffix = appName + "/" + appId + "/" + fileDstPath;
    Path dst = new Path(fs.getHomeDirectory(), suffix);
    if (fileSrcPath == null) {
      FSDataOutputStream ostream = null;
      try {
        ostream = FileSystem.create(fs, dst, new FsPermission((short) 0710));
        ostream.writeUTF(resources);
      } finally {
        IOUtils.closeQuietly(ostream);
      }
    } else {
      fs.copyFromLocalFile(new Path(fileSrcPath), dst);
    }

    FileStatus scFileStatus = fs.getFileStatus(dst);
    LocalResource scRsrc = LocalResource.newInstance(
        ConverterUtils.getYarnUrlFromURI(dst.toUri()), LocalResourceType.FILE,
        LocalResourceVisibility.APPLICATION, scFileStatus.getLen(),
        scFileStatus.getModificationTime());
    localResources.put(fileDstPath, scRsrc);
  }

  private void addHdfsResource(FileSystem fs, String fileSrcPath,
      String fileDstPath, int appId, Map<String, LocalResource> localResources)
      throws IOException {
    Path p = new Path(fileSrcPath);
    FileStatus scFileStatus = fs.getFileStatus(p);
    LocalResource scRsrc = LocalResource.newInstance(
        ConverterUtils.getYarnUrlFromURI(scFileStatus.getPath().toUri()),
        LocalResourceType.FILE, LocalResourceVisibility.APPLICATION,
        scFileStatus.getLen(), scFileStatus.getModificationTime());
    LOG.info("adding resource: " + scRsrc.toString());
    LOG.info("to: " + fileDstPath);
    localResources.put(fileDstPath, scRsrc);
  }

  private boolean fileExist(String filePath) {
    if (filePath == null) {
      LOG.fatal("null string passed to fileExist");
      return false;
    }
    File f = new File(filePath);
    return f.exists() && !f.isDirectory();
  }

}
