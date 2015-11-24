package org.apache.hadoop.yarn.applications.zmq.am;
import java.nio.ByteBuffer;
import java.net.InetAddress;
import java.util.concurrent.ExecutorService;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.yarn.api.records.Container;
import org.apache.hadoop.yarn.client.api.async.NMClientAsync;



class MyContainer { 
	private static final Log LOG = LogFactory.getLog(MyContainer.class);
	private CommandFileGenerator commandFileGenerator_;
	public MyContainerStatus status_ = MyContainerStatus.Null;
	public Container container;
	public String type;
	public String name_;
	public int id_;
	public String cmdFileName;
	public NMCallbackHandler containerListener;
	public ByteBuffer allTokens;
	public Configuration conf;
	public NMClientAsync nmClientAsync;
	private ExecutorService containerLauncher;
	
	public MyContainer(String type,
			int id, CommandFileGenerator commandFileGenerator,
			NMCallbackHandler containerListener,
			ByteBuffer allTokens,
			Configuration conf,
			NMClientAsync nmClientAsync,
			ExecutorService containerLauncher) {
		this.type = type;
		id_ = id;
		name_ = "(" + this.type + "," + id_ + ")";
		commandFileGenerator_ = commandFileGenerator;
		status_ = MyContainerStatus.Null;
		cmdFileName = "_" + type + id_ + ".cmd";
		this.containerListener = containerListener;
		this.allTokens = allTokens;
		this.conf = conf;
		this.nmClientAsync = nmClientAsync;
		this.containerLauncher = containerLauncher;
	}
	
	public void UpdateStatus(MyContainerStatus status) {
		status_ = status;
	}
	
	public void Start(Container container) {
		this.container = container;
		status_ = MyContainerStatus.Starting;
		
      try {
      	InetAddress address = InetAddress.getByName(container.getNodeId().getHost()); 
    	commandFileGenerator_.GenerateCommandFile(type,
    			  id_, cmdFileName, address.getHostAddress());
      } catch (Exception e) {
        LOG.error("Failed to generate " + name_ +
        		"'s command file " + cmdFileName + " "
          + container.getNodeId().getHost() + "\n"
          );
      }

      LOG.info("Launching " + name_ + " on "
          + container.getId().toString()
          + ", containerNode=" + container.getNodeId().getHost()
          + ":" + container.getNodeId().getPort());
      
      LaunchContainerRunnable runnableLaunchContainer = new LaunchContainerRunnable(this);
      synchronized (containerLauncher) {
        containerLauncher.execute(runnableLaunchContainer);
      }
	}
	
	public static enum MyContainerStatus {
		Null, Starting, Running, Failed, Succeed;
	}
};	