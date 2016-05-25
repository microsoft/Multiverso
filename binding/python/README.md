# Requirements
I presume you followed the [README](../../README.md) and have build multiverso successfully.


# Run tests
```
cd ./multiverso
python -m unittest test.TestMultiversoTables.test_array
python -m unittest test.TestMultiversoTables.test_matrix
```


# Run logistic regression example with multi-process
```
mpirun -np 4 python ./examples/theano/logistic_regression.py
```
