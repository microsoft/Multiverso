// The interface is following the design of Piccolo paper 
// Refer to http://piccolo.news.cs.nyu.edu/piccolo.pdf for details.

#ifndef MULTIVERSO_TABLE_INTERFACE_H_
#define MULTIVERSO_TABLE_INTERFACE_H_

namespace multiverso {

// Interface for Multiverso shared parameters. Key is the type of global shared
// Value is the parameters for machine learning tasks. Can be Matrix, Vector,
// Hash Table, Hybrid Table...etc. 
template <typename Key, typename Value>
class TableInterface {
public:

  virtual void Init();

  void Clear();

  void Get(const Key& key, Value* value);

  virtual AsyncGet(const Key& key, Value* value) = 0;

  void Put(const Key& key, Value* value);

  void Update(const Key& key, Value* value) = 0;
  
  void Flush();

  virtual void Partition();

protected:
  virtual ~TableInterface() {}
  
};

} // namespace multiverso

#endif // MULTIVERSO_TABLE_INTERFACE_H_