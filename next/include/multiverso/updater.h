#ifndef MULTIVERSO_UPDATER_H_
#define MULTIVERSO_UPDATER_H_

namespace multiverso {
 
// IUpdater is the abstract for server updater.
// TODO(feiga): abstract the common server update logic out
//              leave the server table only the data structure for model storage
//              implement different algorithm such as normal asgd, adam, or 
//              others as different updater. Thus can be re-used by array, 
//              matrix, and others. 
class IUpdater {

};

}

#endif // MULTIVERSO_UPDATER_H_