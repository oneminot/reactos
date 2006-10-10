/*
 * Austin---Astonishing Universal Search Tree Interface Novelty
 * Copyright (C) 2000 Kaz Kylheku <kaz@ashi.footprints.net>
 * Copyright (C) 2000 Carl van Tast <vanTast@netway.at>
 *
 * Free Software License:
 *
 * All rights are reserved by the author, with the following exceptions:
 * Permission is granted to freely reproduce and distribute this software,
 * possibly in exchange for a fee, provided that this copyright notice appears
 * intact. Permission is also granted to adapt this software to produce
 * derivative works, as long as the modified versions carry this copyright
 * notice and additional notices stating that the work has been modified.
 * This source code may be translated into executable form and incorporated
 * into proprietary software; there is no requirement for such software to
 * contain a copyright notice related to this source.
 *
 * $Id: avl.c,v 1.3 2000/01/12 02:37:02 kaz Exp $
 * $Name: austin_0_2 $
 */
/*
 * Modified for use in ReactOS by arty
 */

#include "udict.h"
#include "tree.h"
#include "macros.h"

#define balance Balance
#define BALANCED udict_balanced
#define LEFTHEAVY udict_leftheavy
#define RIGHTHEAVY udict_rightheavy
#define EQUAL GenericEqual
#define LESS GenericLessThan
#define GREATER GenericGreaterThan

void avl_init(udict_t *ud)
{
	ud->sentinel.left = ud->sentinel.right = ud->sentinel.parent = 
		&ud->sentinel;
}

void RotateLeft(udict_node_t **top)
{
	udict_node_t *parent = *top;
	udict_node_t *child = parent->right;

	child->parent = parent->parent;
	parent->right = child->left;
	parent->right->parent = parent;     /* may change sentinel.parent */
	child->left = parent;
	parent->parent = child;
	*top = child;
}/*RotateLeft*/

void RotateRight(udict_node_t **top)
{
	udict_node_t *parent = *top;
	udict_node_t *child = parent->left;

	child->parent = parent->parent;
	parent->left = child->right;
	parent->left->parent = parent;     /* may change sentinel.parent */
	child->right = parent;
	parent->parent = child;
	*top = child;
}/*RotateRight*/

void FixBalance(udict_node_t **pnode, udict_avl_balance_t bal)
{
	udict_node_t *node = *pnode;
	udict_node_t *child;
	udict_node_t *grandchild;

	if (node->balance == BALANCED) {
		node->balance = bal;
	}/*if*/
	else if (node->balance != bal) {
		node->balance = BALANCED;
	}/*elsif*/
	else {
		assert (node->balance == bal);

		if (bal == LEFTHEAVY) {
			child = node->left;
			if (child->balance == LEFTHEAVY) {
				node->balance = BALANCED;
				child->balance = BALANCED;
				RotateRight(pnode);
			}/*if*/
			else if (child->balance == BALANCED) {
				/* only possible after delete */
				node->balance = LEFTHEAVY;
				child->balance = RIGHTHEAVY;
				RotateRight(pnode);
			}/*elsif*/
			else {
				assert (child->balance == RIGHTHEAVY);

				grandchild = child->right;
				if (grandchild->balance == LEFTHEAVY) {
					node->balance = RIGHTHEAVY;
					child->balance = BALANCED;
				}/*if*/
				else if (grandchild->balance == RIGHTHEAVY) {
					node->balance = BALANCED;
					child->balance = LEFTHEAVY;
				}/*elsif*/
				else {
					node->balance = BALANCED;
					child->balance = BALANCED;
				}/*else*/
				grandchild->balance = BALANCED;
				RotateLeft(&node->left);
				RotateRight(pnode);
			}/*else*/
		}/*if*/
		else {
			assert (bal == RIGHTHEAVY);

			child = node->right;
			if (child->balance == RIGHTHEAVY) {
				node->balance = BALANCED;
				child->balance = BALANCED;
				RotateLeft(pnode);
			}/*if*/
			else if (child->balance == BALANCED) {
				/* only possible after delete */
				node->balance = RIGHTHEAVY;
				child->balance = LEFTHEAVY;
				RotateLeft(pnode);
			}/*elsif*/
			else {
				assert (child->balance == LEFTHEAVY);

				grandchild = child->left;
				if (grandchild->balance == RIGHTHEAVY) {
					node->balance = LEFTHEAVY;
					child->balance = BALANCED;
				}/*if*/
				else if (grandchild->balance == LEFTHEAVY) {
					node->balance = BALANCED;
					child->balance = RIGHTHEAVY;
				}/*elsif*/
				else {
					node->balance = BALANCED;
					child->balance = BALANCED;
				}/*else*/
				grandchild->balance = BALANCED;
				RotateRight(&node->right);
				RotateLeft(pnode);
			}/*else*/
		}/*else*/
	}/*else*/
}/*FixBalance*/

int Insert(udict_t *ud, udict_node_t *what, udict_node_t **where, udict_node_t *parent)
{
	udict_node_t *here = *where;
	int result;

	if (here == tree_null_priv(ud)) {
		*where = what;
		what->parent = parent;
		return 1;                       /* higher than before */
	}/*if*/
	else {
		result = ud->compare(ud, key(what), key(here));	

		if (result == LESS) {
			if (Insert(ud, what, &here->left, here)) {
				/*
				** now left side is higher than before
				*/
				FixBalance(where, LEFTHEAVY);
				return ((*where)->balance != BALANCED);
			}/*if*/
		}/*if*/
		else {
			if (Insert(ud, what, &here->right, here)) {
				/*
				** now right side is higher than before
				*/
				FixBalance(where, RIGHTHEAVY);
				return ((*where)->balance != BALANCED);
			}/*if*/
		}/*else*/
	}/*else*/
	return 0;                           /* height not changed */
}/*Insert*/

void avl_insert_node(udict_t *ud, udict_node_t *node)
{
	udict_node_t *nil = tree_null_priv(ud);

	node->left = nil;
	node->right = nil;
	node->balance = BALANCED;

	if (Insert(ud, node, &nil->left, nil)) {
		nil->balance = LEFTHEAVY;
	}/*if*/

	ud->nodecount++;
}

void avl_delete_node(udict_t *ud, udict_node_t *node)
{
	udict_node_t *nil = tree_null_priv(ud);
	udict_node_t *swap;
	udict_node_t *child;
	udict_node_t *parent;

	udict_tree_delete(ud, node, &swap, &child);

#ifndef NDEBUG
	if (swap == node) {
		/*
		** node had 0 or 1 child,
		** child moved up to node's place
		*/
		if (child != nil) {
			assert ((child->left == nil) && (child->right == nil));
			assert (child->balance == BALANCED);
		}/*if*/
	}/*if*/
	else {
		/*
		** node had 2 children,
		** swap was node's successor (in node's right subtree),
		** swap has been inserted in node's place,
		** child was swap->right,
		** child has been moved to swap's place
		*/
		if (child != nil) {
			assert ((child->left == nil) && (child->right == nil));
			assert (child->balance == BALANCED);
		}/*if*/
	}/*else*/
#endif
	swap->balance = node->balance;

	/*
	** In either case, child has been moved to the next higher level.
	** So the balance of its new parent has to be checked.
	** Note, that child->parent points to the node we are interested in,
	** even if child == nil.
	*/

	parent = child->parent;

	if (parent == nil) {
		/* root has been deleted */
		if (child == nil) {
			parent->balance = BALANCED;
		}/*if*/
	}/*if*/

	while (parent != nil) {
		if ((parent->left == nil) && (parent->right == nil)) {
			assert (child == nil);
			assert (parent->balance != BALANCED);
			parent->balance = BALANCED;
			/* propagate height reduction to upper level */
		}/*if*/
		else {
			udict_node_t **pparent;
			if (parent == parent->parent->left)
				pparent = &parent->parent->left;
			else
				pparent = &parent->parent->right;

			if (child == parent->left) {
				/* reduce parent's left height */
				FixBalance(pparent, RIGHTHEAVY);
			}/*if*/
			else {
				assert (child == parent->right);
				/* reduce parent's right height */
				FixBalance(pparent, LEFTHEAVY);
			}/*else*/

			/*
			** parent and child are not valid now,
			** pparent may point to new root of subtree
			*/
			parent = *pparent;
		}/*else*/

		/* if subtree is balanced, then height is less than before */
		if (parent->balance == BALANCED) {
			child = parent;
			parent = child->parent;
		}/*if*/
		else
			break;
	}/*while*/
}/*avl_delete*/

int avl_search(udict_t *ud, void *_key, udict_node_t *here, udict_node_t **where)
{
	int result;

	result = ud->compare(ud, _key, key(here));	

	if (result == EQUAL) {
		*where = here;
		return TableFoundNode;
	}

	if (result == LESS) {
		if( here->left == &ud->sentinel ) {
			*where = here;
			return TableInsertAsLeft;
		}
		return avl_search(ud, _key, here->left, where);
	}/*if*/
	else {
		if( here->right == &ud->sentinel ) {
			*where = here;
			return TableInsertAsRight;
		}
		return avl_search(ud, _key, here->right, where);
	}/*else*/
}

int avl_is_nil(udict_t *ud, udict_node_t *node)
{
	return &ud->sentinel == node;
}

udict_node_t *avl_first(udict_t *ud)
{
	return udict_tree_first(ud);
}

udict_node_t *avl_last(udict_t *ud)
{
	return udict_tree_last(ud);
}

udict_node_t *avl_next(udict_t *ud, udict_node_t *prev)
{
	return udict_tree_next(ud, prev);
}
