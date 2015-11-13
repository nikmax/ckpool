/*
 * Copyright 1995-2015 Andrew Smith
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#include "ktree.h"

static const int dbg = 0;
#define DBG	if (dbg != 0) printf

#define FAIL(fmt, ...) do \
	{ \
		quithere(1, fmt KTREE_FFL, ##__VA_ARGS__, KTREE_FFL_PASS); \
	} while (0);

#define RED_RED         true
#define RED_BLACK       false

#define Yo true
#define No false

static K_NODE nil[1] = { { Yo, RED_BLACK, NULL, NULL, NULL, NULL, 0 } };

static K_NODE *_new_knode(KTREE_FFL_ARGS)
{
	K_NODE *node = (K_NODE *)malloc(sizeof(*node));

	if (node == NULL)
		FAIL("%s", "node OOM");

	node->isNil = Yo;
	node->red = RED_BLACK;
	node->parent = nil;
	node->left = nil;
	node->right = nil;
	node->data = NULL;
	node->test = 0;

	return node;
}

K_TREE *_new_ktree(cmp_t (*cmp_funct)(K_ITEM *, K_ITEM *), KTREE_FFL_ARGS)
{
	K_TREE *tree = (K_TREE *)malloc(sizeof(*tree));

	if (tree == NULL)
		FAIL("%s", "tree OOM");

	tree->root = _new_knode(KTREE_FFL_PASS);

	tree->cmp_funct = cmp_funct;

	return tree;
}

static K_NODE *new_data(K_ITEM *data, KTREE_FFL_ARGS)
{
	K_NODE *knode = (K_NODE *)malloc(sizeof(*knode));

	if (knode == NULL)
		FAIL("%s", "OOM");

	knode->isNil = No;
	knode->red = RED_RED;
	knode->parent = nil;
	knode->left = nil;
	knode->right = nil;
	knode->data = data;
	knode->test = 0;

	return knode;
}

static int bCount = 0;
static long bTestValue = 0;
static long nilTestValue = 0;
static long lrpTestValue = 0;

static long testValue = 1231230L;

static long getTestValue()
{
	return ++testValue;
}

static void show_ktree(K_NODE *node, char *path, int pos, char *(*dsp_funct)(K_ITEM *))
{
	char col;

	if (node->isNil == Yo)
		return;

	if (node->left->isNil == No)
	{
		path[pos] = 'L';
		path[pos+1] = '\0';
		show_ktree(node->left, path, pos+1, dsp_funct);
	}

	path[pos] = '\0';

	if (node->red == RED_RED)
		col = 'R';
	else
	// if (node->red == RED_BLACK)
		col = 'B';

	printf(" %c %s=%s\n", col, path, dsp_funct(node->data));

	if (node->right->isNil == No)
	{
		path[pos] = 'R';
		path[pos+1] = '\0';
		show_ktree(node->right, path, pos+1, dsp_funct);
	}
}

void _dump_ktree(K_TREE *tree, char *(*dsp_funct)(K_ITEM *), KTREE_FFL_ARGS)
{
	char buf[42424];

	printf("dump:\n");
	if (tree->root->isNil == No)
	{
		buf[0] = 'T';
		buf[1] = '\0';
		show_ktree(tree->root, buf, 1, dsp_funct);
	}
	else
		printf(" Empty tree\n");
}

void _dsp_ktree(K_LIST *list, K_TREE *tree, char *filename, char *msg, KTREE_FFL_ARGS)
{
	K_TREE_CTX ctx[1];
	K_ITEM *item;
	FILE *stream;
	struct tm tm;
	time_t now_t;
	char stamp[128];

	if (!list->dsp_func)
		FAIL("%s", "NULLDSP NULL dsp_func");

	now_t = time(NULL);
	localtime_r(&now_t, &tm);
	snprintf(stamp, sizeof(stamp),
			"[%d-%02d-%02d %02d:%02d:%02d]",
			tm.tm_year + 1900,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);

	stream = fopen(filename, "ae");
	if (!stream)
	{
		fprintf(stderr, "%s %s() failed to open '%s' (%d) %s",
				stamp, __func__, filename, errno, strerror(errno));
		return;
	}

	if (msg)
		fprintf(stream, "%s %s\n", stamp, msg);
	else
		fprintf(stream, "%s Dump of tree '%s':\n", stamp, list->name);

	if (tree->root->isNil == No)
	{
		item = first_in_ktree(tree, ctx);
		while (item)
		{
			list->dsp_func(item, stream);
			item = next_in_ktree(ctx);
		}
		fprintf(stream, "End\n\n");
	}
	else
		fprintf(stream, "Empty ktree\n\n");

	fclose(stream);
}

static int nilTest(K_NODE *node, char *msg, int depth, int count, K_NODE *nil2, KTREE_FFL_ARGS)
{
	if (node->isNil == Yo || node == nil2)
	{
		if (node == nil2 && node->isNil == No)
			FAIL("NIL2NOTNIL '%s' depth=%d count=%d", msg, depth, count);

		if (node != nil && node != nil2)
			FAIL("NOTNILNIL2 '%s' depth=%d count=%d", msg, depth, count);

		if (node->red == RED_RED)
			FAIL("NIRED '%s' depth=%d count=%d", msg, depth, count);

		if (node != nil2 && node->parent != NULL)
			FAIL("NILPARENT '%s' depth=%d count=%d", msg, depth, count);

		if (node != nil2 && node->left != NULL)
			FAIL("NILLEFT '%s' depth=%d count=%d", msg, depth, count);

		if (node == nil2 && node->left != nil)
			FAIL("NIL2LEFT '%s' depth=%d count=%d", msg, depth, count);

		if (node != nil2 && node->right != NULL)
			FAIL("NILRIGHT '%s' depth=%d count=%d", msg, depth, count);

		if (node == nil2 && node->right != nil)
			FAIL("NIL2RIGHT '%s' depth=%d count=%d", msg, depth, count);

		if (node->data != NULL)
			FAIL("NIDATA '%s' depth=%d count=%d", msg, depth, count);
	}
	else
	{
		count++;

		if (node->parent == NULL)
			FAIL("NOPAR '%s' depth=%d count=%d", msg, depth, count);

		if (node->left == NULL)
			FAIL("NOLEFT '%s' depth=%d count=%d", msg, depth, count);

		if (node->right == NULL)
			FAIL("NORIGHT '%s' depth=%d count=%d", msg, depth, count);

		if (node->data == NULL)
			FAIL("NODATA '%s' depth=%d count=%d", msg, depth, count);

		if (node->test != nilTestValue)
			node->test = nilTestValue;
		else
			FAIL("NILTESTVALUE '%s' depth=%d count=%d", msg, depth, count);

		count = nilTest(node->left, msg, depth+1, count, nil2, KTREE_FFL_PASS);
		count = nilTest(node->right, msg, depth+1, count, nil2, KTREE_FFL_PASS);
	}

	return(count);
}

static void bTest(K_NODE *cur, char *msg, int count, KTREE_FFL_ARGS)
{
	if (cur->red != RED_RED)
		count++;
	else
	{
		if (cur->left->red == RED_RED)
			FAIL("CURLR '%s' count=%d", msg, count);

		if (cur->right->red == RED_RED)
			FAIL("CURRR '%s' count=%d", msg, count);
	}

	if (cur->isNil == Yo)
	{
		if (bCount == 0)
			bCount = count;

		if (count != bCount)
			FAIL("BCOUNT '%s' count=%d bCount=%d", msg, count, bCount);
	}
	else
	{
		if (cur->test != bTestValue)
			cur->test = bTestValue;
		else
			FAIL("BTESTVALUE '%s' count=%d", msg, count);

		bTest(cur->left, msg, count, KTREE_FFL_PASS);
		bTest(cur->right, msg, count, KTREE_FFL_PASS);
	}
}

static void bTestInit(K_TREE *tree, char *msg, KTREE_FFL_ARGS)
{
	bCount = 0;
	bTestValue = getTestValue();
	bTest(tree->root, msg, 0, KTREE_FFL_PASS);
}

static void lrpTest(K_NODE *node, char *msg, KTREE_FFL_ARGS)
{
	if (node->test != lrpTestValue)
		node->test = lrpTestValue;
	else
		FAIL("LRPTESTVALUE '%s'", msg);

	if (node->left->isNil == No)
	{
		if (node->left->parent != node)
			FAIL("LRPTESTL '%s'", msg);

		lrpTest(node->left, msg, KTREE_FFL_PASS);
	}

	if (node->right->isNil == No)
	{
		if (node->right->parent != node)
			FAIL("LRPTESTR '%s'", msg);

		lrpTest(node->right, msg, KTREE_FFL_PASS);
	}
}

static __maybe_unused void check_ktree(K_TREE *tree, char *msg, K_NODE *nil2, int debugNil, int debugLRP, int debugColor, KTREE_FFL_ARGS)
{
	if (tree->root->isNil == Yo)
		return;

	if (debugNil)
	{
		nilTestValue = getTestValue();
		nilTest(tree->root, msg, 1, 0, nil2, KTREE_FFL_PASS);
	}

	if (debugLRP && tree->root->isNil == No)
	{
		lrpTestValue = getTestValue();
		lrpTest(tree->root, msg, KTREE_FFL_PASS);
	}

	if (debugColor && tree->root->isNil == No)
		bTestInit(tree, msg, KTREE_FFL_PASS);
}

static K_ITEM *_first_in_knode(K_NODE *node, K_TREE_CTX *ctx, KTREE_FFL_ARGS)
{
	if (node->isNil == No)
	{
		while (node->left->isNil == No)
			node = node->left;

		*ctx = node;
		return(node->data);
	}

	*ctx = NULL;
	return(NULL);
}

K_ITEM *_first_in_ktree(K_TREE *tree, K_TREE_CTX *ctx, KTREE_FFL_ARGS)
{
	return _first_in_knode(tree->root, ctx, KTREE_FFL_PASS);
}

static K_ITEM *_last_in_knode(K_NODE *node, K_TREE_CTX *ctx, KTREE_FFL_ARGS)
{
	if (node->isNil == No)
	{
		while (node->right->isNil == No)
			node = node->right;

		*ctx = node;
		return(node->data);
	}

	*ctx = NULL;
	return(NULL);
}

K_ITEM *_last_in_ktree(K_TREE *tree, K_TREE_CTX *ctx, KTREE_FFL_ARGS)
{
	return _last_in_knode(tree->root, ctx, KTREE_FFL_PASS);
}

K_ITEM *_next_in_ktree(K_TREE_CTX *ctx, KTREE_FFL_ARGS)
{
	K_NODE *parent;
	K_NODE *knode = (K_NODE *)(*ctx);

	if (knode->isNil == No)
	{
		if (knode->right->isNil == No)
			return(_first_in_knode(knode->right, ctx, KTREE_FFL_PASS));
		else
		{
			parent = knode->parent;
			while (parent->isNil == No && knode == parent->right)
			{
				knode = parent;
				parent = parent->parent;
			}
			if (parent->isNil == No)
			{
				*ctx = parent;
				return(parent->data);
			}
		}
	}

	*ctx = NULL;
	return(NULL);
}

K_ITEM *_prev_in_ktree(K_TREE_CTX *ctx, KTREE_FFL_ARGS)
{
	K_NODE *parent;
	K_NODE *knode = (K_NODE *)(*ctx);

	if (knode->isNil == No)
	{
		if (knode->left->isNil == No)
			return(_last_in_knode(knode->left, ctx, KTREE_FFL_PASS));
		else
		{
			parent = knode->parent;
			while (parent->isNil == No && knode == parent->left)
			{
				knode = parent;
				parent = parent->parent;
			}
			if (parent->isNil == No)
			{
				*ctx = parent;
				return(parent->data);
			}
		}
	}

	*ctx = NULL;
	return(NULL);
}

static K_NODE *left_rotate(K_NODE *root, K_NODE *about)
{
	K_NODE *rotate;

	rotate = about->right;
	about->right = rotate->left;

	if (rotate->left->isNil == No)
		rotate->left->parent = about;

	rotate->parent = about->parent;

	if (about->parent->isNil == Yo)
		root = rotate;
	else
	{
		if (about == about->parent->left)
			about->parent->left = rotate;
		else
			about->parent->right = rotate;
	}

	rotate->left = about;
	about->parent = rotate;

	return(root);
}

static K_NODE *right_rotate(K_NODE *root, K_NODE *about)
{
	K_NODE *rotate;

	rotate = about->left;
	about->left = rotate->right;

	if (rotate->right->isNil == No)
		rotate->right->parent = about;

	rotate->parent = about->parent;

	if (about->parent->isNil == Yo)
		root = rotate;
	else
		if (about == about->parent->right)
			about->parent->right = rotate;
		else
			about->parent->left = rotate;

	rotate->right = about;
	about->parent = rotate;

	return(root);
}

void _add_to_ktree(K_TREE *tree, K_ITEM *data, KTREE_FFL_ARGS)
{
	K_NODE *knode;
	K_NODE *x, *y;
	K_NODE *pp;
	cmp_t cmp;

	if (tree == NULL)
		FAIL("%s", "ADDNULL add tree is NULL");

//check_ktree(tree, ">add", NULL, 1, 1, 1, KTREE_FFL_PASS);

	if (tree->root->parent != nil && tree->root->parent != NULL)
		FAIL("%s", "ADDROOT add tree->root isn't the root");

	knode = new_data(data, KTREE_FFL_PASS);

	if (tree->root->isNil == Yo)
	{
		if (tree->root != nil)
			free(tree->root);

		tree->root = knode;
	}
	else
	{
		x = tree->root;
		y = nil;
		while (x->isNil == No)
		{
			y = x;
			if ((cmp = tree->cmp_funct(knode->data, x->data)) < 0)
				x = x->left;
			else
				x = x->right;
		}
		knode->parent = y;
		if (cmp < 0)
			y->left = knode;
		else
			y->right = knode;

		x = knode;
		while (x != tree->root && x->parent->red == RED_RED)
		{
			pp = x->parent->parent;
			if (x->parent == pp->left)
			{
				y = pp->right;
				if (y->red == RED_RED)
				{
					x->parent->red = RED_BLACK;
					y->red = RED_BLACK;
					pp->red = RED_RED;
					x = pp;
				}
				else
				{
					if (x == x->parent->right)
					{
						x = x->parent;
						tree->root = left_rotate(tree->root, x);
						pp = x->parent->parent;
					}
					x->parent->red = RED_BLACK;
					pp->red = RED_RED;
					tree->root = right_rotate(tree->root, pp);
				}
			}
			else
			{
				y = pp->left;
				if (y->red == RED_RED)
				{
					x->parent->red = RED_BLACK;
					y->red = RED_BLACK;
					pp->red = RED_RED;
					x = pp;
				}
				else
				{
					if (x == x->parent->left)
					{
						x = x->parent;
						tree->root = right_rotate(tree->root, x);
						pp = x->parent->parent;
					}
					x->parent->red = RED_BLACK;
					pp->red = RED_RED;
					tree->root = left_rotate(tree->root, pp);
				}
			}
		}
	}
	tree->root->red = RED_BLACK;

//check_ktree(tree, "<add", NULL, 1, 1, 1, KTREE_FFL_PASS);
}

K_ITEM *_find_in_ktree(K_TREE *tree, K_ITEM *data, K_TREE_CTX *ctx, KTREE_FFL_ARGS)
{
	K_NODE *knode;
	cmp_t cmp = -1;

	if (tree == NULL)
		FAIL("%s", "FINDNULL find tree is NULL");

	if (tree->root == NULL)
		FAIL("%s", "FINDNULL find tree->root is NULL");

	knode = tree->root;

	while (knode->isNil == No && cmp != 0)
	{
		if ((cmp = tree->cmp_funct(knode->data, data)))
		{
			if (cmp > 0)
				knode = knode->left;
			else
				knode = knode->right;
		}
	}

	if (knode->isNil == No)
	{
		*ctx = knode;
		return(knode->data);
	}
	else
	{
		*ctx = NULL;
		return(NULL);
	}
}

K_ITEM *_find_after_in_ktree(K_TREE *tree, K_ITEM *data, K_TREE_CTX *ctx, KTREE_FFL_ARGS)
{
	K_NODE *knode, *old = NULL;
	cmp_t cmp = -1, oldcmp = -1;

	if (tree == NULL)
		FAIL("%s", "FINDNULL find_after tree is NULL");

	if (tree->root == NULL)
		FAIL("%s", "FINDNULL find_after tree->root is NULL");

	knode = tree->root;

	while (knode->isNil == No && cmp != 0)
	{
		if ((cmp = tree->cmp_funct(knode->data, data)))
		{
			old = knode;
			oldcmp = cmp;
			if (cmp > 0)
				knode = knode->left;
			else
				knode = knode->right;
		}
	}

	if (knode->isNil == No)
	{
		*ctx = knode;
		return next_in_ktree(ctx);
	}
	else
	{
		if (old)
		{
			if (oldcmp > 0)
			{
				*ctx = old;
				return(old->data);
			}

			*ctx = old;
			return next_in_ktree(ctx);
		}

		*ctx = NULL;
		return(NULL);
	}
}

K_ITEM *_find_before_in_ktree(K_TREE *tree, K_ITEM *data, K_TREE_CTX *ctx, KTREE_FFL_ARGS)
{
	K_NODE *knode, *old = NULL;
	cmp_t cmp = 1, oldcmp = 1;

	if (tree == NULL)
		FAIL("%s", "FINDNULL find_before tree is NULL");

	if (tree->root == NULL)
		FAIL("%s", "FINDNULL find_before tree->root is NULL");

	knode = tree->root;

	while (knode->isNil == No && cmp != 0)
	{
		if ((cmp = tree->cmp_funct(knode->data, data)))
		{
			old = knode;
			oldcmp = cmp;
			if (cmp > 0)
				knode = knode->left;
			else
				knode = knode->right;
		}
	}

	if (knode->isNil == No)
	{
		*ctx = knode;
		return prev_in_ktree(ctx);
	}
	else
	{
		if (old)
		{
			if (oldcmp < 0)
			{
				*ctx = old;
				return(old->data);
			}

			*ctx = old;
			return prev_in_ktree(ctx);
		}

		*ctx = NULL;
		return(NULL);
	}
}

static K_NODE *removeFixup(K_NODE *root, K_NODE *fix)
{
	K_NODE *w = NULL;

	while (fix != root && fix->red != RED_RED)
	{
		if (fix == fix->parent->left)
		{
			w = fix->parent->right;
			if (w->red == RED_RED)
			{
				w->red = RED_BLACK;
				fix->parent->red = RED_RED;
				root = left_rotate(root, fix->parent);
				w = fix->parent->right;
			}

			if (w->left->red != RED_RED && w->right->red != RED_RED)
			{
				w->red = RED_RED;
				fix = fix->parent;
			}
			else
			{
				if (w->right->red != RED_RED)
				{
					w->left->red = RED_BLACK;
					w->red = RED_RED;
					root = right_rotate(root, w);
					w = fix->parent->right;
				}

				w->red = fix->parent->red;
				fix->parent->red = RED_BLACK;
				w->right->red = RED_BLACK;
				root = left_rotate(root, fix->parent);
				fix = root;
			}
		}
		else
		{
			w = fix->parent->left;
			if (w->red == RED_RED)
			{
				w->red = RED_BLACK;
				fix->parent->red = RED_RED;
				root = right_rotate(root, fix->parent);
				w = fix->parent->left;
			}

			if (w->right->red != RED_RED && w->left->red != RED_RED)
			{
				w->red = RED_RED;
				fix = fix->parent;
			}
			else
			{
				if (w->left->red != RED_RED)
				{
					w->right->red = RED_BLACK;
					w->red = RED_RED;
					root = left_rotate(root, w);
					w = fix->parent->left;
				}

				w->red = fix->parent->red;
				fix->parent->red = RED_BLACK;
				w->left->red = RED_BLACK;
				root = right_rotate(root, fix->parent);
				fix = root;
			}
		}
	}

	fix->red = RED_BLACK;

	return root;
}

// Does this work OK when you remove the last element in the tree?
// It should return the root as 'nil'
void _remove_from_ktree(K_TREE *tree, K_ITEM *data, K_TREE_CTX *ctx, KTREE_FFL_ARGS)
{
	K_TREE_CTX tmpctx[1];
	K_NODE *found;
	K_ITEM *fdata;
	K_NODE *x, *y, *nil2;
	// cmp_t cmp;
	int yred;

//check_ktree(tree, ">remove", NULL, 1, 1, 1, KTREE_FFL_PASS);

	if (tree == NULL)
		FAIL("%s", "REMNULL remove tree is NULL");

	if (tree->root == NULL)
		FAIL("%s", "REMNULL remove tree->root is NULL");

	if (tree->root->isNil == Yo)
	{
		*ctx = NULL;
		return;
	}

	if (tree->root->parent->isNil == No)
		FAIL("%s", "REMROOT remove tree->root isn't the root");

	fdata = find_in_ktree(tree, data, ctx);

	if (fdata == NULL)
		return;

	if (tree->cmp_funct(fdata, data) != 0)
		FAIL("%s", "BADFIND cmp(found, remove) != 0");

	found = *ctx;

	x = nil;
	y = nil;

	if (found->left->isNil == Yo || found->right->isNil == Yo)
		y = found;
	else
	{
		*tmpctx = found;
		next_in_ktree(tmpctx);
		y = *tmpctx;
	}

	yred = y->red;

	if (y->left->isNil == No && y->right->isNil == No)
		FAIL("%s", "REMBADY remove error");

	if (y->left->isNil == Yo)
		x = y->right;
	else
		x = y->left;

	if (x != nil)
		nil2 = NULL;
	else
	{
		nil2 = _new_knode(KTREE_FFL_PASS);
		x = nil2;
	}

	x->parent = y->parent;

	if (x->parent->isNil == Yo)
		tree->root = x;
	else
	{
		if (x->parent->left == y)
			x->parent->left = x;
		else
			x->parent->right = x;
	}

	if (y != found)
	{
		if (tree->root == found)
			tree->root = y;

		if (x == found)
			x = y;

		y->red = found->red;
		y->parent = found->parent;

		if (y->parent->isNil == No)
		{
			if (y->parent->left == found)
				y->parent->left = y;
			else
				y->parent->right = y;
		}

		y->left = found->left;
		if (y->left->isNil == No || y->left == nil2)
			y->left->parent = y;

		y->right = found->right;
		if (y->right->isNil == No || y->right == nil2)
			y->right->parent = y;
	}

	if (yred != RED_RED)
		tree->root = removeFixup(tree->root, x);

	if (nil2 != NULL)
	{
		if (nil2->parent->isNil == No && nil2->parent->left == nil2)
			nil2->parent->left = nil;

		if (nil2->parent->isNil == No && nil2->parent->right == nil2)
			nil2->parent->right = nil;

		if (tree->root == nil2)
			tree->root = nil;

/*
if (dbg != 0)
{
 if (nil2->left != nil)
 {
DBG("@remove nil2->left wasn't nil!!!\n");
 }
 if (nil2->right != nil)
 {
DBG("@remove nil2->right wasn't nil!!!\n");
 }
 cmp = 0;
 fdata = first_in_ktree(tree, tmpctx);;
 while (fdata != NULL)
 {
	cmp++;
	x = *tmpctx;
	if (x == nil2)
	{
DBG("@remove found nil2 in ktree %d!!!\n", (int)cmp);
	}
	else
		if (x->left == nil2)
		{
DBG("@remove found nil2 in ktree(left) %d!!!\n", (int)cmp);
		}
		else
			if (x->right == nil2)
			{
DBG("@remove found nil2 in ktree(right) %d!!!\n", (int)cmp);
			}

	fdata = next_in_ktree(tmpctx);;
 }
}
*/
		free(nil2);
	}

/*
if (dbg != 0)
{
 cmp = 0;
 fdata = first_in_ktree(tree, tmpctx);;
 while (fdata != NULL)
 {
	if (tree->cmp_funct(fdata, tree->root->data) < 0)
		cmp--;
	else
		cmp++;

	fdata = next_in_ktree(tmpctx);;
 }
 if (cmp < -10 || cmp > 10)
 {
DBG("@remove after balance=%d :(\n", (int)cmp);
 }
}
*/


//check_ktree(tree, "<remove", NULL, 1, 1, 1, KTREE_FFL_PASS);

	return;
}

void _remove_from_ktree_free(K_TREE *root, K_ITEM *data, KTREE_FFL_ARGS)
{
	K_TREE_CTX ctx[1];

	_remove_from_ktree(root, data, ctx, KTREE_FFL_PASS);

	if (*ctx)
		free(*ctx);
}

static void free_ktree_sub(K_NODE *knode, void (*free_funct)(void *))
{
	if (knode != NULL && knode != nil)
	{
		if (knode->data != NULL && free_funct)
			free_funct(knode->data);

		free_ktree_sub(knode->left, free_funct);
		free_ktree_sub(knode->right, free_funct);

		free(knode);
	}
}

void _free_ktree(K_TREE *tree, void (*free_funct)(void *), KTREE_FFL_ARGS)
{
	if (tree == NULL)
		FAIL("%s", "FREENULL free NULL tree");

	if (tree->root->parent != NULL && tree->root->parent != nil)
		FAIL("%s", "FREENOTROOT free tree->root not root");

	free_ktree_sub(tree->root, free_funct);
}
