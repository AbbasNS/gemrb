/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GUI/GUIScriptInterface.h"
#include "PythonHelpers.h"
#include "GUI/Window.h"

using namespace GemRB;

PythonCallback::PythonCallback(PyObject *Function)
	: Function(Function)
{
	if (Function && PyCallable_Check(Function)) {
		Py_INCREF(Function);
	} else {
		Function = NULL;
	}
}

PythonCallback::~PythonCallback()
{
	if (Py_IsInitialized()) {
		Py_XDECREF(Function);
	}
}

void PythonCallback::operator() () const
{
	if (!Function || !Py_IsInitialized()) {
		return;
	}
	CallPython(Function);
}

namespace GemRB {

template <>
void PythonObjectCallback<Control*, void>::operator() (Control* ctrl) const
{
	if (!ctrl || !Function || !Py_IsInitialized()) {
		return;
	}

	PyObject *args = NULL;
	PyObject* func_code = PyObject_GetAttrString(Function, "func_code");
	PyObject* co_argcount = PyObject_GetAttrString(func_code, "co_argcount");
	const long count = PyInt_AsLong(co_argcount);
	if (count) {
		assert(count <= 2);
		PyObject* obj = gs->ConstructObjectForScriptable(ctrl->GetScriptingRef());
		if (count > 1) {
			args = Py_BuildValue("(Ni)", obj, ctrl->GetValue());
		} else {
			args = Py_BuildValue("(N)", obj);
		}
	}
	Py_DECREF(func_code);
	Py_DECREF(co_argcount);
	CallPython(Function, args);
}

template <>
bool PythonObjectCallback<WindowKeyPress&, bool>::operator() (WindowKeyPress &wkp) const {
	if (!Function || !Py_IsInitialized()) {
		return false;
	}

	PyObject *args = PyTuple_Pack(2, PyInt_FromLong(wkp.key),
																	 PyInt_FromLong(wkp.mod));
	long result = CallPythonWithReturn(Function, args);

	return result > 0;
}

static PyObject* CallPythonObject(PyObject *Function, PyObject *args) {
	if (!Function) {
		return NULL;
	}

	PyObject *ret = PyObject_CallObject(Function, args);
	Py_XDECREF( args );
	if (ret == NULL) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
		return NULL;
	}

	return ret;
}

bool CallPython(PyObject *Function, PyObject *args)
{
	PyObject *ret = CallPythonObject(Function, args);

	if(ret) {
		Py_DECREF(ret);

		return true;
	} else {
		return false;
	}
}

long CallPythonWithReturn(PyObject *Function, PyObject *args) {
	PyObject *ret = CallPythonObject(Function, args);

	if(ret) {
		long value = PyInt_AsLong(ret);
		Py_DECREF(ret);

		return value;
	}

	return -1;
}

}
