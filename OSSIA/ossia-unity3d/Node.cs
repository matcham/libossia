﻿using System.Runtime;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System;

namespace Ossia {
	public class Node 
	{
		internal IntPtr ossia_node = IntPtr.Zero;
		Address ossia_address = null;
		bool updating = false;
		Network.ossia_node_callback node_remove_callback = null;
		IntPtr node_ossia_remove_callback = IntPtr.Zero;

		private void CleanupCallback()
		{
			if (ossia_node != IntPtr.Zero && node_ossia_remove_callback != IntPtr.Zero) {
				Network.ossia_node_remove_deleting_callback (ossia_node, node_ossia_remove_callback);

				ossia_node = IntPtr.Zero;
				ossia_address = null;
				node_remove_callback = null;
				node_ossia_remove_callback = IntPtr.Zero;
			}
		}

		public Node(IntPtr n)
		{
			ossia_node = n;

			node_remove_callback = new Network.ossia_node_callback((IntPtr ctx, IntPtr node) => CleanupCallback());
			IntPtr intptr_delegate = Marshal.GetFunctionPointerForDelegate (node_remove_callback);
			node_ossia_remove_callback = Network.ossia_node_add_deleting_callback(ossia_node, intptr_delegate, (IntPtr)0);

			var addr = Network.ossia_node_get_address (ossia_node);
			if (addr != IntPtr.Zero) {
				ossia_address = new Ossia.Address (addr);
			}
		}

		~Node()
		{
			CleanupCallback ();		
		}

		public string GetName()
		{
			if (ossia_node != IntPtr.Zero) {
				IntPtr nameptr = Network.ossia_node_get_name (ossia_node);
				if (nameptr != IntPtr.Zero) {
					string name = Marshal.PtrToStringAnsi (nameptr);
					Network.ossia_string_free (nameptr);
					return name;
				}
			}
			return null;
		}

		public Node AddChild (string name)
		{
			if (ossia_node != IntPtr.Zero) {
				var cld = Network.ossia_node_add_child (ossia_node, name);
				if (cld != IntPtr.Zero) {
					return new Node (cld);
				}
			}
			return null;
		}

		public void RemoveChild(Node child)
		{
			if (child != null && ossia_node != IntPtr.Zero) {
				Network.ossia_node_remove_child (ossia_node, child.ossia_node);
			}
		}

		public int ChildSize()
		{
			if(ossia_node != IntPtr.Zero)
			  return Network.ossia_node_child_size(ossia_node);
			return 0;
		}

		public Node GetChild(int child)
		{
			if (ossia_node != IntPtr.Zero) {
				var cld = Network.ossia_node_get_child (ossia_node, child);
				if (cld != IntPtr.Zero) {
					return new Node (cld);
				}
			}
			return null;
		}

		public Address CreateAddress(ossia_type type)
		{
			if(ossia_node != IntPtr.Zero && ossia_address == null)
				ossia_address = new Address (Network.ossia_node_create_address (ossia_node, type));
			return ossia_address;
		}

		public void RemoveAddress()
		{
			if (ossia_node != IntPtr.Zero) {
				Network.ossia_node_remove_address (ossia_node, ossia_address.ossia_address);
			}
			ossia_address = null;
		}

		public IntPtr GetNode() {
			return ossia_node;
		}
		public Ossia.Address GetAddress() {
			return ossia_address;
		}

		public bool GetValueUpdating() {
			return updating;
		}
		public void SetValueUpdating(bool b)
		{
			if (ossia_node != IntPtr.Zero) {
				if (ossia_address != null) {
					ossia_address.SetValueUpdating (b);
				}

				for (int i = 0; i < ChildSize (); i++) {
					Node child = GetChild (i);
					child.SetValueUpdating (b);
				}

				updating = b;
			}
		}

		//! Usage: Node.CreateNode(myRootNode, "/foo/baz/bar");
		public static Node CreateNode(Node root, string s)
		{
			if (root.ossia_node != IntPtr.Zero) {
				IntPtr p = Network.ossia_node_create(root.ossia_node, s);
				return new Node(p);
			}
			return null;
		}
		//! Usage: Node.FindNode(myRootNode, "/foo/baz/bar");
		public static Node FindNode(Node root, string s)
		{
			if (root.ossia_node != IntPtr.Zero) {
				IntPtr p = Network.ossia_node_find (root.ossia_node, s);
				if (p != IntPtr.Zero)
					return new Node (p);
			}
		    return null;
		}

		//! Usage: Node.FindPattern(myRootNode, "/foo/baz.*");
		public static unsafe Node[] FindPattern(Node root, string s)
		{
			if (root.ossia_node != IntPtr.Zero) {
				IntPtr data;
				UIntPtr size;

				Network.ossia_node_find_pattern (root.ossia_node, s, out data, out size);
				int sz = (int)size;
				if (sz == 0)
					return null;
				
				void** ptr = (void**)data.ToPointer();
				Node[] arr = new Node[sz];
				for (int i = 0; i < sz; i++) {
					arr[i] = new Node(new IntPtr(ptr[i]));
				}
				Network.ossia_node_array_free (data);
				return arr;
			}
			return null;
		}

		//! Usage: Node.FindPattern(myRootNode, "/foo/baz.*");
		public static unsafe Node[] CreatePattern(Node root, string s)
		{
			if (root.ossia_node != IntPtr.Zero) {
				IntPtr data;
				UIntPtr size;

				Network.ossia_node_create_pattern (root.ossia_node, s, out data, out size);
				int sz = (int)size;
				if (sz == 0)
					return null;
				
				void** ptr = (void**)data.ToPointer();
				Node[] arr = new Node[sz];
				for (int i = 0; i < sz; i++) {
					arr[i] = new Node(new IntPtr(ptr[i]));
				}
				Network.ossia_node_array_free (data);
				return arr;
			}
			return null;
		}
	}
}
