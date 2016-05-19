#!/usr/bin/env python
# coding:utf8

from theano.tensor.basic import TensorType, _tensor_py_operators
from theano.compile import SharedVariable
from theano.gof import Variable, utils
import numpy
import multiverso as mv


class MVSharedVariable(SharedVariable):
    def __init__(self, name, type, value, strict,
                 allow_downcast=None, container=None):
        super(MVSharedVariable, self).__init__(name, type, value, strict,
             allow_downcast, container)
        self._mv_array = mv.ArrayTableHandler(self.get_value().size)

        self._mv_array.add(self.get_value().reshape((-1,)))

        # I restore a copy of value. It will be used for calculate the update
        # for multiverso when calling mv_sync
        self._last_mv_data = self.get_value(borrow=False)

    def mv_sync(self):
        '''
        This will add the delta of SharedVariable to parameter server and then
        get the latest value in multiverso
        '''
        # because multiverso always use add method to sync value, the delta
        # will be the difference of the current value of last synced value
        self._mv_array.add(self.get_value() - self._last_mv_data)

        self.set_value(self._mv_array.get().reshape(self.get_value().shape))
        self._last_mv_data = self.get_value(borrow=False)


class MVTensorSharedVariable(_tensor_py_operators, MVSharedVariable):
    pass


def mv_shared(value, name=None, strict=False, allow_downcast=None, **kwargs):
    """Return a MVSharedVariable Variable, initialized with a copy or
    reference of `value`.

    This function iterates over constructor functions to find a
    suitable MVSharedVariable subclass.  The suitable one is the first
    constructor that accept the given value.  See the documentation of
    :func:`shared_constructor` for the definition of a contructor
    function.

    This function is meant as a convenient default.  If you want to use a
    specific shared variable constructor, consider calling it directly.

    .. attribute:: constructors

    A list of shared variable constructors that will be tried in reverse
    order.

    Notes
    -----
    By passing kwargs, you effectively limit the set of potential constructors
    to those that can accept those kwargs.

    Some shared variable have ``borrow`` as extra kwargs.
    `See <http://deeplearning.net/software/theano/tutorial/aliasing.\
    html#borrowing-when-creating-shared-variables>`_ for details.

    Some shared variable have ``broadcastable`` as extra kwargs. As shared
    variable shapes can change, all dimensions default to not being
    broadcastable, even if ``value`` has a shape of 1 along some dimension.
    This parameter allows you to create for example a `row` or `column` 2d
    tensor.

    """

    try:
        if isinstance(value, Variable):
            raise TypeError("Shared variable constructor needs numeric "
                            "values and not symbolic variables.")

        for ctor in reversed(mv_shared.constructors):
            try:
                var = ctor(value, name=name, strict=strict,
                           allow_downcast=allow_downcast, **kwargs)
                utils.add_tag_trace(var)
                return var
            except TypeError:
                continue
            # This may happen when kwargs were supplied
            # if kwargs were given, the generic_constructor won't be callable.
            #
            # This was done on purpose, the rationale being that if kwargs
            # were supplied, the user didn't want them to be ignored.

    except MemoryError as e:
        e.args = e.args + ('you might consider'
                           ' using \'theano.mv_shared(..., borrow=True)\'',)
        raise

    raise TypeError('No suitable SharedVariable constructor could be found.'
                    ' Are you sure all kwargs are supported?'
                    ' We do not support the parameter dtype or type.'
                    ' value="%s". parameters="%s"' %
                    (value, kwargs))

mv_shared.constructors = []


def shared_constructor(ctor, remove=False):
    if remove:
        mv_shared.constructors.remove(ctor)
    else:
        mv_shared.constructors.append(ctor)
    return ctor


@shared_constructor
def mv_tensor_constructor(value, name=None, strict=False, allow_downcast=None,
                       borrow=False, broadcastable=None, target='cpu'):
    """
    MVSharedVariable Constructor for TensorType.

    Notes
    -----
    Regarding the inference of the broadcastable pattern...
    The default is to assume that the value might be resized in any
    dimension, so the default broadcastable is ``(False,)*len(value.shape)``.
    The optional `broadcastable` argument will override this default.

    """
    if target != 'cpu':
        raise TypeError('not for cpu')

    if not isinstance(value, numpy.ndarray):
        raise TypeError()

    # if no broadcastable is given, then the default is to assume that
    # the value might be resized in any dimension in the future.
    #
    if broadcastable is None:
        broadcastable = (False,) * len(value.shape)
    type = TensorType(value.dtype, broadcastable=broadcastable)
    return MVTensorSharedVariable(type=type,
                                value=numpy.array(value, copy=(not borrow)),
                                name=name,
                                strict=strict,
                                allow_downcast=allow_downcast)


# TODO implement the following constructor and related SharedVariable
# <function theano.compile.sharedvalue.generic_constructor>
# <function theano.tensor.sharedvar.scalar_constructor>
# <function theano.tensor.shared_randomstreams.randomstate_constructor>
