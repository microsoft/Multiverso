package org.apache.hadoop.yarn.applications.zmq.am;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;

import org.apache.hadoop.yarn.applications.zmq.DSConstants;

public class CommandFileGenerator {
  CommandFileGenerator(boolean isVerboseOn, int workerServerPort,
      int numWorkers, String workerArgs, String serverArgs) {
    isVerboseOn_ = isVerboseOn;
    workerServerPort_ = workerServerPort;
    numWorkers_ = numWorkers;
    workerArgs_ = workerArgs;
    serverArgs_ = serverArgs;
  }

  public void GenerateCommandFile(String type, int id, String fileName, String ip) throws Exception {
    if (type == DSConstants.WORKER)
      GenerateWorkerCommandFile(id, fileName);
    else
      GenerateServerCommandFile(id, ip, fileName);
  }

  /**
  * start.bat endpointlist workerId workerServerPort workerArgs
  */
  private void GenerateWorkerCommandFile(int workerId, String fileName) throws IOException {
    BufferedWriter out = new BufferedWriter(new FileWriter(
        fileName));
    if (DSConstants.isWindow) {
    	  out.write("cd " + DSConstants.WORKERDIR + "\n");
	    if (isVerboseOn_) {
	      out.write("dir /s \n");
	      out.write("whoami\n");
	      out.write("type " + " " + DSConstants.STARTFILE + " " + "\n");
	      out.write("type " + DSConstants.ENDPOINTLIST + "\n");
	    }
	
	    out.write("call " + DSConstants.STARTFILE + " "
	        + DSConstants.ENDPOINTLIST + " " + workerId + " " 
	        + workerServerPort_ + " " + workerArgs_ + " 2>&1 \n");
	    out.write("echo worker " + workerId + " exit with code %errorlevel%\n");
	    out.write("exit /b %errorlevel%\n");
	    out.close();
    } else {
    	out.write("cd " + DSConstants.WORKERDIR + "\n");
	    if (isVerboseOn_) {
		      out.write("ls \n");
		      out.write("whoami\n");
		      out.write("cat " + " " + DSConstants.STARTFILE + " " + "\n");
		      out.write("cat " + DSConstants.ENDPOINTLIST + "\n");
		    }
		
		    out.write("./" + DSConstants.STARTFILE + " "
		        + DSConstants.ENDPOINTLIST + " " + workerId + " " 
		        + workerServerPort_ + " " + workerArgs_ + " 2>&1 \n");
		    out.close();
    }
  }

  /**
  * start.bat serverId numWorkers ip:port serverArgs
  */
  private void GenerateServerCommandFile(int serverId, String ip, String fileName) throws IOException {
    BufferedWriter out = new BufferedWriter(
        new FileWriter(fileName));
    if (DSConstants.isWindow) {
    	out.write("cd " + DSConstants.SERVERDIR + "\n");
	    if (isVerboseOn_) {
	      out.write("dir /s \n");
	      out.write("whoami\n");
	      out.write("type " + " " + DSConstants.STARTFILE + " \n");
	    }
	
	    out.write("call " + DSConstants.STARTFILE + " "
	        + serverId + " " + numWorkers_ + " "+ ip + ":"
	        + workerServerPort_ + " " + serverArgs_ + " 2>&1 \n");
	    out.write("echo server " + serverId +" exit with code %errorlevel%\n");
	    out.write("exit /b %errorlevel%\n");
	    out.close();
    } else {
    	out.write("cd " + DSConstants.SERVERDIR + "\n");
	    if (isVerboseOn_) {
		      out.write("ls \n");
		      out.write("whoami\n");
		      out.write("cat " + " " + DSConstants.STARTFILE + " \n");
		    }
		
		    out.write("./" + DSConstants.STARTFILE + " "
		        + serverId + " " + numWorkers_ + " "+ ip + ":"
		        + workerServerPort_ + " " + serverArgs_ + " 2>&1 \n");
		    out.close();
    }
  }

  private boolean isVerboseOn_;
  private int workerServerPort_;
  private int numWorkers_;
  private String workerArgs_;
  private String serverArgs_;
}