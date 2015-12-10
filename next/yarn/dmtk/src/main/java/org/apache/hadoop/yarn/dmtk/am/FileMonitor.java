package org.apache.hadoop.yarn.dmtk.am;
import java.io.*;
import java.net.*;
import java.util.concurrent.atomic.AtomicBoolean;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.yarn.dmtk.DSConstants;

public class FileMonitor {
  private static final Log LOG = LogFactory.getLog(FileMonitor.class);
  
  public FileMonitor(AtomicBoolean _isWorkDone) {
    isWorkDone = _isWorkDone;
  }

  private AtomicBoolean isWorkDone;
  private int hasRead = 0;

  public void watchFile(String filePath, String address, int port) {
    int nReconnect = 0;
    while (true) {
      try {
        System.err.println("Connecting to Client at " + address);
        Socket socket = new Socket(address, port);
        System.err.println("Connected to client");
        PrintWriter os = new PrintWriter(socket.getOutputStream());
        File file = new File(filePath);
        BufferedReader reader = null;
        boolean lastRead = false; // if mpi is done, we read for the last time
        while (true) {
          reader = new BufferedReader(new FileReader(file));
          int line = 0;
          String temp = null;
          while ((temp = reader.readLine()) != null) {
            if (line >= hasRead) {
              os.println(temp);
              os.flush();
              hasRead++;
            }
            line++;
          }
          reader.close();
          if (lastRead) {
            os.println(DSConstants.AM_END_OUTPUT);
            os.flush();
            LOG.info("read till end of log, exit now");
            break;
          }
          if (isWorkDone.get()) {
            LOG.info("isWorkDone is set, will read to end and exit");
            lastRead = true;
          }
          reader.close();
          Thread.sleep(1000);
        }
        os.close();
        socket.close();
        break;
      } catch (Exception e) {
        e.printStackTrace();
        nReconnect++;
        if (nReconnect > 3) {
          break;
        }
      }
    }
    LOG.info("FileMonitor shutting down");
  }
}
