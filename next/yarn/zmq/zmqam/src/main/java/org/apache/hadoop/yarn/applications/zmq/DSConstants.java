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

package org.apache.hadoop.yarn.applications.zmq;

import org.apache.hadoop.classification.InterfaceAudience;
import org.apache.hadoop.classification.InterfaceStability;

/**
 * Constants used in both Client and Application Master
 */
@InterfaceAudience.Public
@InterfaceStability.Unstable
public class DSConstants {
  public static final int DEFAULT_ALLOCATE_TIMEOUT = 120;  // if cannot allocate enough containers in 2min, kill  
  public static final int DEFAULT_EXECUTE_TIMEOUT = 30 * 24 * 60 * 60;   // if executed more than 24h, kill job
  public static final String AM_END_OUTPUT = "### End of AM stdout ###";

  public static final String WORKER = "Worker";
  public static final String SERVER = "Server";
  public static final String ENDPOINTLIST = "_endpointlist";
  public static final String MACHINELISTFILE = "_machinelist";
  public static final String SERVERDIR = "_serverdir";
  public static final String LOCALSERVERDIR = "server";
  public static final String WORKERDIR = "_workerdir";
  public static final String LOCALWORKERDIR = "worker";
  public static final String CMD = "_cmd.cmd";
  public static final String JARFILE = "_jar.jar";
  public static final String STARTFILE = "start.bat";


  // for yarnclient
  public static final String OPT_HELP = "h";
  public static final String OPT_QUEUE = "q";
  public static final String OPT_APPNAME = "appname";

  public static final String OPT_AMCLIENTPORT = "p";
  public static final String OPT_WORKERSERVERPORT = "wsp";
  public static final String OPT_PROCESSMEMORY = "m";
  public static final String OPT_PROCESSCORES = "c";
  public static final String OPT_NUMWORKERS = "wn";
  public static final String OPT_NUMSERVERS = "sn";
  public static final String OPT_WORKERARGS = "wa";
  public static final String OPT_SERVERARGS = "sa";   
  public static final String OPT_DEBUG = "d";
  public static final String OPT_VERBOSE = "v";
  public static final String OPT_SYNCHRONOUS = "s";
  public static final String OPT_ALLOCATE_TIMEOUT = "alloctime";
  public static final String OPT_EXECUTE_TIMEOUT = "exectime";
  public static final String OPT_JAR_FILE = "jar_file";

  // only for application master
  public static final boolean isWindow = true;
  public static final String ENV_AMCLIENTPORT = "_amclientport";
  public static final String ENV_WORKERSERVERPORT = "_workerserverport";  
  public static final String ENV_NUMWORKERS = "_numworkers";
  public static final String ENV_NUMSERVERS = "_numservers";   
  public static final String ENV_PROCESSMEMORY = "_processmemory";
  public static final String ENV_PROCESSCORES = "_processcores";

  public static final String ENV_DEBUG = "_debug";
  public static final String ENV_WORKERARGS = "_workerargs";
  public static final String ENV_SERVERARGS = "_serverargs";
  public static final String ENV_VERBOSE = "_verbose";
  public static final String ENV_SYNCHRONOUS = "_synchronous";
  public static final String ENV_HDFS_APP_DIR = "_hdfsappdir";
  public static final String ENV_CLIENTADDR = "_clientaddr";
  public static final String ENV_ALLOCATE_TIMEOUT = "_allocatetimeout";
  public static final String ENV_EXECUTE_TIMEOUT = "_executetimeout";
}
