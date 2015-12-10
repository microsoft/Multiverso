package org.apache.hadoop.yarn.dmtk.am;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.yarn.api.ApplicationConstants;
import org.apache.hadoop.yarn.api.ContainerManagementProtocol;
import org.apache.hadoop.yarn.api.records.Container;
import org.apache.hadoop.yarn.api.records.ContainerLaunchContext;
import org.apache.hadoop.yarn.api.records.LocalResource;
import org.apache.hadoop.yarn.api.records.LocalResourceType;
import org.apache.hadoop.yarn.api.records.LocalResourceVisibility;
import org.apache.hadoop.yarn.dmtk.DSConstants;
import org.apache.hadoop.yarn.util.ConverterUtils;
import org.apache.hadoop.yarn.util.Records;

/**
   * Thread to connect to the {@link ContainerManagementProtocol} and launch the
   * container that will execute the shell command.
   */
public class LaunchContainerRunnable implements Runnable {
	private static final Log LOG = LogFactory.getLog(LaunchContainerRunnable.class);
    // Allocated container
    MyContainer container_;

    /**
     * @param lcontainer
     *          Allocated container
     * @param containerListener
     *          Callback handler of the container
     */
    public LaunchContainerRunnable(MyContainer container) {
      container_ = container;
    }

    @Override
    /**
     * Connects to CM, sets up container launch context 
     * for shell command and eventually dispatches the container 
     * start request to the CM. 
     */
    public void run() {
      LOG.info("Setting up container "
    		  + container_.name_ + " for containerid="
    		  + container_.container.getId());
      ContainerLaunchContext ctx = Records
          .newRecord(ContainerLaunchContext.class);

      String folderName = DSConstants.SERVERDIR;
      if (container_.type == DSConstants.WORKER) {
        folderName = DSConstants.WORKERDIR;
      }

      // Set the local resources
      Map<String, LocalResource> localResources = new HashMap<String, LocalResource>();
      // add jar, dll and env file
      try {
        FileSystem localFs = FileSystem.get(container_.conf);
        addToLocalResources(localFs, container_.cmdFileName,
            DSConstants.CMD, localResources, null);
        addFolderToLocalResources(localFs, folderName, folderName,
          localResources, null);
      } catch (IOException e) {
        e.printStackTrace();
      }
      ctx.setLocalResources(localResources);

      List<String> commands = new ArrayList<String>();

      if (DSConstants.isWindow) {
	      commands.add(DSConstants.CMD + " 1>"
	          + ApplicationConstants.LOG_DIR_EXPANSION_VAR + "/stdout" + " 2>"
	          + ApplicationConstants.LOG_DIR_EXPANSION_VAR + "/stderr");
      } else {
    	  commands.add("./" + DSConstants.CMD + " 1>"
    	          + ApplicationConstants.LOG_DIR_EXPANSION_VAR + "/stdout" + " 2>"
    	          + ApplicationConstants.LOG_DIR_EXPANSION_VAR + "/stderr");
      }
      
      ctx.setCommands(commands);

      // Set up tokens for the container too. Today, for normal shell
      // commands, the container in distribute-shell doesn't need any
      // tokens. We are populating them mainly for NodeManagers to be
      // able to download any files in the distributed file-system.
      // The tokens are otherwise also useful in cases, for e.g., when
      // one is running a "hadoop dfs" command inside the distributed shell.
      ctx.setTokens(container_.allTokens.duplicate());

      container_.containerListener.addContainer(container_.container.getId(), container_.container);
      container_.nmClientAsync.startContainerAsync(container_.container, ctx);
    }

    private void addFolderToLocalResources(FileSystem fs, String fileSrcPath,
        String fileDstPath, Map<String, LocalResource> localResources,
        String resources) throws IOException {
      File file = new File(fileSrcPath);
      if (file.isDirectory()) {
        File [] files = file.listFiles();
        for (File f : files) {
          String dstFile = fileDstPath + File.separator
                  + f.getName();
           addFolderToLocalResources(fs, f.getAbsolutePath(), dstFile,
            localResources, null);
        }
      }
      else {
        String suffix = container_.container.getId() + "/" + fileDstPath;
        Path dst = new Path(fs.getHomeDirectory(), suffix);
        fs.copyFromLocalFile(new Path(fileSrcPath), dst);
        FileStatus scFileStatus = fs.getFileStatus(dst);
        LocalResource scRsrc = LocalResource.newInstance(
            ConverterUtils.getYarnUrlFromURI(dst.toUri()),
            LocalResourceType.FILE, LocalResourceVisibility.APPLICATION,
            scFileStatus.getLen(), scFileStatus.getModificationTime());
        LOG.info("adding " + container_.name_ + "'s file: " + scRsrc.toString() + "to: " + fileDstPath);
        localResources.put(fileDstPath, scRsrc);
      }
    }

    private void addToLocalResources(FileSystem fs, String fileSrcPath,
        String fileDstPath, Map<String, LocalResource> localResources,
        String resources) throws IOException {
      String suffix = container_.container.getId() + "/" + fileDstPath;
      Path dst = new Path(fs.getHomeDirectory(), suffix);
      fs.copyFromLocalFile(new Path(fileSrcPath), dst);
      FileStatus scFileStatus = fs.getFileStatus(dst);
      LocalResource scRsrc = LocalResource.newInstance(
          ConverterUtils.getYarnUrlFromURI(dst.toUri()),
          LocalResourceType.FILE, LocalResourceVisibility.APPLICATION,
          scFileStatus.getLen(), scFileStatus.getModificationTime());
      LOG.info("adding dependency: " + scRsrc.toString() + "to: " + fileDstPath);
      localResources.put(fileDstPath, scRsrc);
    }

    private void addHdfsResource(FileSystem fs, Path fileSrcPath,
        String fileDstPath, Map<String, LocalResource> localResources)
        throws IOException {
      FileStatus scFileStatus = fs.getFileStatus(fileSrcPath);
      LocalResource scRsrc = LocalResource.newInstance(
          ConverterUtils.getYarnUrlFromURI(scFileStatus.getPath().toUri()),
          LocalResourceType.FILE, LocalResourceVisibility.APPLICATION,
          scFileStatus.getLen(), scFileStatus.getModificationTime());
      LOG.info("adding resource: " + scRsrc.toString() + "to: " + fileDstPath);
      localResources.put(fileDstPath, scRsrc);
    }
  }
