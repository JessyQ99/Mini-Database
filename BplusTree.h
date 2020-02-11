#ifndef _BPLUS_TREE_H
#define _BPLUS_TREE_H

#include "BufferManager.h"
#include "DS.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include "IndexString.h"

// 定义B+树可用的内存空间的大小
#define MIN_CACHE_NUM 5

// 预处理宏，在编译时就已经生效与类无关
// 使用宏减少代码量
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

#define ADDR_STR_WIDTH 16
#define offset_ptr(node) ((char *) (node) + sizeof(*node))
#define key(node) ((key_t *)offset_ptr(node))
#define data(node) ((long long *)(offset_ptr(node) + _max_entries * _var_size))
#define sub(node) ((off_t *)(offset_ptr(node) + (_max_order - 1) * _var_size))

// 定义B+树中所需要用到的枚举类型
enum {
    INVALID_OFFSET = 0xdeadbeef,
};

enum {
    BPLUS_TREE_LEAF,
    BPLUS_TREE_NON_LEAF = 1,
};

enum {
    LEFT_SIBLING,
    RIGHT_SIBLING = 1,
};

struct list_head {
    list_head *prev, *next;
};

struct bplus_tree {
    char *caches;
    int used[MIN_CACHE_NUM];
    char filename[1024];
    int fd;
    int level;
    off_t root;
    off_t file_size;
    list_head free_blocks;
};

typedef struct bplus_node {
    off_t self;
    off_t parent;
    off_t prev;
    off_t next;
    int type;
    /* If leaf node, it specifies  count of entries,
    * if non-leaf node, it specifies count of children(branches) */
    int children;
} bplus_node;

typedef struct free_block {
    list_head link;
    off_t offset;
} free_block;

template<typename key_t>
class BplusTree{
public:
    BplusTree(int size):_var_size(size){

    }
    ~BplusTree(){

    }
    // 从B+树中按key值搜索一个值
    long long bplus_tree_get( bplus_tree *tree, key_t key);

    // 根据data参数的不同，分别可以向B+树中插入一个值和删除一个值
    int bplus_tree_put( bplus_tree *tree, key_t key, long long data);

    // 范围搜索函数，返回符合条件的数目
    int bplus_tree_get_range(bplus_tree *tree, char *filename, key_t key1, int beqmin, int beqmax, int pivot, BufferManager &TM);

    // 初始化B+树，建立文件并返回指向根节点的指针
    bplus_tree *bplus_tree_init(char *filename, int block_size);

    // 结束使用时调用，将对应的B+树写回文件中
    void bplus_tree_deinit( bplus_tree *tree);

    // 打开一个B+树文件
    int bplus_open(char *filename);

    // 关闭一个B+树文件
    void bplus_close(int fd);

    // 返回一个恰好小于或者大于指定值的指针
    long long bplus_tree_get_con(bplus_tree *tree, key_t key1, int pivot);
private:
    // 一个node最大的大小，用位表示
    int _block_size;

    // 一个节点最大的接入口数
    int _max_entries;

    // 节点的秩
    int _max_order;

    // 一个数据需要的byte数
    int _var_size;

    // 根据偏移量向文件中储存B+树的相关信息
    ssize_t offset_store(int fd, off_t offset);
    // 从文件中读取B+树相关的信息
    off_t offset_load(int fd);
    void hex_to_str(off_t offset, char *buf, int len);
    off_t str_to_hex(char *c, int len);

    // 删除函数
    int bplus_tree_delete( bplus_tree *tree, key_t key);
    int leaf_remove( bplus_tree *tree,  bplus_node *leaf, key_t key);
    void leaf_simple_remove( bplus_tree *tree,  bplus_node *leaf, int remove);
    void leaf_merge_from_right( bplus_tree *tree,  bplus_node *leaf,  bplus_node *right);
    void leaf_shift_from_right( bplus_tree *tree,  bplus_node *leaf,  bplus_node *right,  bplus_node *parent, int parent_key_index);
    void leaf_merge_into_left( bplus_tree *tree,  bplus_node *leaf,  bplus_node *left, int parent_key_index, int remove);
    void leaf_shift_from_left( bplus_tree *tree,  bplus_node *leaf,  bplus_node *left,  bplus_node *parent, int parent_key_index, int remove);
    void non_leaf_remove( bplus_tree *tree,  bplus_node *node, int remove);
    void non_leaf_simple_remove( bplus_tree *tree,  bplus_node *node, int remove);
    void non_leaf_merge_from_right( bplus_tree *tree,  bplus_node *node,  bplus_node *right,  bplus_node *parent, int parent_key_index);
    void non_leaf_shift_from_right( bplus_tree *tree,  bplus_node *node,  bplus_node *right,  bplus_node *parent, int parent_key_index);
    void non_leaf_merge_into_left( bplus_tree *tree,  bplus_node *node,  bplus_node *left,  bplus_node *parent, int parent_key_index, int remove);
    void non_leaf_shift_from_left( bplus_tree *tree,  bplus_node *node,  bplus_node *left,  bplus_node *parent, int parent_key_index, int remove);
    int sibling_select( bplus_node *l_sib,  bplus_node *r_sib,  bplus_node *parent, int i);
    // 插入函数
    int bplus_tree_insert( bplus_tree *tree, key_t key, long long data);
    int leaf_insert( bplus_tree *tree,  bplus_node *leaf, key_t key, long long data);
    void leaf_simple_insert( bplus_tree *tree,  bplus_node *leaf, key_t key, long long data, int insert);
    key_t leaf_split_right( bplus_tree *tree,  bplus_node *leaf,  bplus_node *right, key_t key, long long data, int insert);
    key_t leaf_split_left( bplus_tree *tree,  bplus_node *leaf,  bplus_node *left, key_t key, long long data, int insert);
    int non_leaf_insert( bplus_tree *tree,  bplus_node *node,  bplus_node *l_ch,  bplus_node *r_ch, key_t key);
    void non_leaf_simple_insert( bplus_tree *tree,  bplus_node *node,  bplus_node *l_ch,  bplus_node *r_ch, key_t key, int insert);
    key_t non_leaf_split_right2( bplus_tree *tree,  bplus_node *node,  bplus_node *right,  bplus_node *l_ch,  bplus_node *r_ch, key_t key, int insert);
    key_t non_leaf_split_right1( bplus_tree *tree,  bplus_node *node,  bplus_node *right,  bplus_node *l_ch,  bplus_node *r_ch, key_t key, int insert);
    key_t non_leaf_split_left( bplus_tree *tree,  bplus_node *node,  bplus_node *left,  bplus_node *l_ch,  bplus_node *r_ch, key_t key, int insert);
    int parent_node_build( bplus_tree *tree,  bplus_node *l_ch,  bplus_node *r_ch, key_t key);
    void right_node_add( bplus_tree *tree,  bplus_node *node,  bplus_node *right);
    void left_node_add( bplus_tree *tree,  bplus_node *node,  bplus_node *left);
    // 搜索函数
    long long bplus_tree_search( bplus_tree *tree, key_t key);
    void sub_node_flush( bplus_tree *tree,  bplus_node *parent, off_t sub_offset);
    void sub_node_update( bplus_tree *tree,  bplus_node *parent, int index,  bplus_node *sub_node);
    void node_delete( bplus_tree *tree,  bplus_node *node,  bplus_node *left,  bplus_node *right);
    off_t new_node_append( bplus_tree *tree,  bplus_node *node);
    void node_flush( bplus_tree *tree,  bplus_node *node);

    // 搜索函数
    bplus_node *node_seek( bplus_tree *tree, off_t offset);
    bplus_node *node_fetch( bplus_tree *tree, off_t offset);
    bplus_node *leaf_new( bplus_tree *tree);
    bplus_node *non_leaf_new( bplus_tree *tree);
    bplus_node *node_new( bplus_tree *tree);
    
    void cache_defer( bplus_tree *tree,  bplus_node *node);
    bplus_node *cache_refer( bplus_tree *tree);
    int parent_key_index( bplus_node *parent, key_t key);
    int key_binary_search( bplus_node *node, key_t target);
    int is_leaf( bplus_node *node);

    // 以下函数用于更便捷的操作指针
    void list_init(list_head *link){
            link->prev = link;
            link->next = link;
    }

    void __list_add(list_head *link,  list_head *prev,  list_head *next){
            link->next = next;
            link->prev = prev;
            next->prev = link;
            prev->next = link;
    }

    void __list_del( list_head *prev,  list_head *next){
            prev->next = next;
            next->prev = prev;
    }

    void list_add( list_head *link,  list_head *prev){
            __list_add(link, prev, prev->next);
    }

    void list_add_tail( list_head *link,  list_head *head){
        __list_add(link, head->prev, head);
    }

    void list_del( list_head *link){
            __list_del(link->prev, link->next);
            list_init(link);
    }

    int list_empty(const  list_head *head){
        return head->next == head;
    }
};

template<typename key_t>
int BplusTree<key_t>::is_leaf(bplus_node *node)
{
        return node->type == BPLUS_TREE_LEAF;
}

template<typename key_t>
int BplusTree<key_t>::key_binary_search( bplus_node *node, key_t target)
{
        key_t *arr = key(node);
        int len = is_leaf(node) ? node->children : node->children - 1;
        int low = -1;
        int high = len;

        while (low + 1 < high) {
                int mid = low + (high - low) / 2;
                if (target > arr[mid]) {
                        low = mid;
                } else {
                        high = mid;
                }
        }

        if (high >= len || arr[high] != target) {
                return -high - 1;
        } else {
                return high;
        }
}

template<typename key_t>
int BplusTree<key_t>::parent_key_index( bplus_node *parent, key_t key)
{
        int index = key_binary_search(parent, key);
        return index >= 0 ? index : -index - 2;
}

template<typename key_t>
bplus_node * BplusTree<key_t>::cache_refer( bplus_tree *tree)
{
        int i;
        for (i = 0; i < MIN_CACHE_NUM; i++) {
                if (!tree->used[i]) {
                        tree->used[i] = 1;
                        char *buf = tree->caches + _block_size * i;
                        return ( bplus_node *) buf;
                }
        }
        assert(0);
}

template<typename key_t>
void BplusTree<key_t>::cache_defer( bplus_tree *tree,  bplus_node *node)
{
        /* return the node cache borrowed from */
        char *buf = (char *) node;
        int i = (buf - tree->caches) / _block_size;
        tree->used[i] = 0;
}

template<typename key_t>
bplus_node * BplusTree<key_t>::node_new( bplus_tree *tree)
{
         bplus_node *node = cache_refer(tree);
        node->self = INVALID_OFFSET;
        node->parent = INVALID_OFFSET;
        node->prev = INVALID_OFFSET;
        node->next = INVALID_OFFSET;
        node->children = 0;
        return node;
}

template<typename key_t>
bplus_node * BplusTree<key_t>::non_leaf_new( bplus_tree *tree)
{
         bplus_node *node = node_new(tree);
        node->type = BPLUS_TREE_NON_LEAF;
        return node;
}

template<typename key_t>
bplus_node * BplusTree<key_t>::leaf_new( bplus_tree *tree)
{
         bplus_node *node = node_new(tree);
        node->type = BPLUS_TREE_LEAF;
        return node;
}

template<typename key_t>
bplus_node * BplusTree<key_t>::node_fetch( bplus_tree *tree, off_t offset)
{
        if (offset == INVALID_OFFSET) {
                return NULL;
        }

        bplus_node *node = cache_refer(tree);
        lseek(tree->fd, offset, SEEK_SET);
        int len = read(tree->fd, node, _block_size);
        lseek(tree->fd, offset, SEEK_SET);
        assert(len == _block_size);
        return node;
}

template<typename key_t>
bplus_node * BplusTree<key_t>::node_seek( bplus_tree *tree, off_t offset)
{
        if (offset == INVALID_OFFSET) {
                return NULL;
        }

        int i;
        for (i = 0; i < MIN_CACHE_NUM; i++) {
                if (!tree->used[i]) {
                        char *buf = tree->caches + _block_size * i;
                        lseek(tree->fd, offset, SEEK_SET);
                        int len = read(tree->fd, buf, _block_size);
                        lseek(tree->fd, offset, SEEK_SET);
                        assert(len == _block_size);
                        return ( bplus_node *) buf;
                }
        }
        assert(0);
}

template<typename key_t>
void BplusTree<key_t>::node_flush( bplus_tree *tree,  bplus_node *node)
{
        if (node != NULL) {
                lseek(tree->fd, node->self, SEEK_SET);
                int len = write(tree->fd, node, _block_size);
                lseek(tree->fd, node->self, SEEK_SET);
                assert(len == _block_size);
                cache_defer(tree, node);
        }
}

template<typename key_t>
off_t BplusTree<key_t>::new_node_append( bplus_tree *tree,  bplus_node *node)
{
        /* assign new offset to the new node */
        if (list_empty(&tree->free_blocks)) {
                node->self = tree->file_size;
                tree->file_size += _block_size;
        } else {
                 free_block *block;
                block = list_first_entry(&tree->free_blocks,  free_block, link);
                list_del(&block->link);
                node->self = block->offset;
                free(block);
        }
        return node->self;
}

template<typename key_t>
void BplusTree<key_t>::node_delete( bplus_tree *tree,  bplus_node *node,
                	 bplus_node *left,  bplus_node *right)
{
        if (left != NULL) {
                if (right != NULL) {
                        left->next = right->self;
                        right->prev = left->self;
                        node_flush(tree, right);
                } else {
                        left->next = INVALID_OFFSET;
                }
                node_flush(tree, left);
        } else {
                if (right != NULL) {
                        right->prev = INVALID_OFFSET;
                        node_flush(tree, right);
                }
        }

        assert(node->self != INVALID_OFFSET);
        free_block *block = (free_block *)malloc(sizeof(*block));
        assert(block != NULL);
        /* deleted blocks can be allocated for other nodes */
        block->offset = node->self;
        list_add_tail(&block->link, &tree->free_blocks);
        /* return the node cache borrowed from */
        cache_defer(tree, node);
}

template<typename key_t>
void BplusTree<key_t>::sub_node_update( bplus_tree *tree,  bplus_node *parent,
                		   int index,  bplus_node *sub_node)
{
        assert(sub_node->self != INVALID_OFFSET);
        sub(parent)[index] = sub_node->self;
        sub_node->parent = parent->self;
        node_flush(tree, sub_node);
}

template<typename key_t>
void BplusTree<key_t>::sub_node_flush( bplus_tree *tree,  bplus_node *parent, off_t sub_offset)
{
         bplus_node *sub_node = node_fetch(tree, sub_offset);
        assert(sub_node != NULL);
        sub_node->parent = parent->self;
        node_flush(tree, sub_node);
}

template<typename key_t>
long long BplusTree<key_t>::bplus_tree_search( bplus_tree *tree, key_t key)
{
        long long ret = -1;
        bplus_node *node = node_seek(tree, tree->root);
        while (node != NULL) {
                int i = key_binary_search(node, key);
                if (is_leaf(node)) {
                        ret = i >= 0 ? data(node)[i] : -1;
                        break;
                } else {
                        if (i >= 0) {
                                node = node_seek(tree, sub(node)[i + 1]);
                        } else {
                                i = -i - 1;
                                node = node_seek(tree, sub(node)[i]);
                        }
                }
        }

        return ret;
}

template<typename key_t>
void BplusTree<key_t>::left_node_add( bplus_tree *tree,  bplus_node *node,  bplus_node *left)
{
        new_node_append(tree, left);

         bplus_node *prev = node_fetch(tree, node->prev);
        if (prev != NULL) {
                prev->next = left->self;
                left->prev = prev->self;
                node_flush(tree, prev);
        } else {
                left->prev = INVALID_OFFSET;
        }
        left->next = node->self;
        node->prev = left->self;
}

template<typename key_t>
void BplusTree<key_t>::right_node_add( bplus_tree *tree,  bplus_node *node,  bplus_node *right)
{
        new_node_append(tree, right);

         bplus_node *next = node_fetch(tree, node->next);
        if (next != NULL) {
                next->prev = right->self;
                right->next = next->self;
                node_flush(tree, next);
        } else {
                right->next = INVALID_OFFSET;
        }
        right->prev = node->self;
        node->next = right->self;
}

template<typename key_t>
int BplusTree<key_t>::parent_node_build( bplus_tree *tree,  bplus_node *l_ch,
                              bplus_node *r_ch, key_t key)
{
        if (l_ch->parent == INVALID_OFFSET && r_ch->parent == INVALID_OFFSET) {
                /* new parent */
                 bplus_node *parent = non_leaf_new(tree);
                key(parent)[0] = key;
                sub(parent)[0] = l_ch->self;
                sub(parent)[1] = r_ch->self;
                parent->children = 2;
                /* write new parent and update root */
                tree->root = new_node_append(tree, parent);
                l_ch->parent = parent->self;
                r_ch->parent = parent->self;
                tree->level++;
                /* flush parent, left and right child */
                node_flush(tree, l_ch);
                node_flush(tree, r_ch);
                node_flush(tree, parent);
                return 0;
        } else if (r_ch->parent == INVALID_OFFSET) {
                return non_leaf_insert(tree, node_fetch(tree, l_ch->parent), l_ch, r_ch, key);
        } else {
                return non_leaf_insert(tree, node_fetch(tree, r_ch->parent), l_ch, r_ch, key);
        }
}

template<typename key_t>
key_t BplusTree<key_t>::non_leaf_split_left( bplus_tree *tree,  bplus_node *node,
                	          bplus_node *left,  bplus_node *l_ch,
                	          bplus_node *r_ch, key_t key, int insert)
{
        int i;
        key_t split_key;

        /* split = [m/2] */
        int split = (_max_order + 1) / 2;

        /* split as left sibling */
        left_node_add(tree, node, left);

        /* calculate split nodes' children (sum as (order + 1))*/
        int pivot = insert;
        left->children = split;
        node->children = _max_order - split + 1;

        /* sum = left->children = pivot + (split - pivot - 1) + 1 */
        /* replicate from key[0] to key[insert] in original node */
        memmove(&key(left)[0], &key(node)[0], pivot * _var_size);
        memmove(&sub(left)[0], &sub(node)[0], pivot * sizeof(off_t));

        /* replicate from key[insert] to key[split - 1] in original node */
        memmove(&key(left)[pivot + 1], &key(node)[pivot], (split - pivot - 1) * _var_size);
        memmove(&sub(left)[pivot + 1], &sub(node)[pivot], (split - pivot - 1) * sizeof(off_t));

        /* flush sub-nodes of the new splitted left node */
        for (i = 0; i < left->children; i++) {
                if (i != pivot && i != pivot + 1) {
                        sub_node_flush(tree, left, sub(left)[i]);
                }
        }

        /* insert new key and sub-nodes and locate the split key */
        key(left)[pivot] = key;
        if (pivot == split - 1) {
                /* left child in split left node and right child in original right one */
                sub_node_update(tree, left, pivot, l_ch);
                sub_node_update(tree, node, 0, r_ch);
                split_key = key;
        } else {
                /* both new children in split left node */
                sub_node_update(tree, left, pivot, l_ch);
                sub_node_update(tree, left, pivot + 1, r_ch);
                sub(node)[0] = sub(node)[split - 1];
                split_key = key(node)[split - 2];
        }

        /* sum = node->children = 1 + (node->children - 1) */
        /* right node left shift from key[split - 1] to key[children - 2] */
        memmove(&key(node)[0], &key(node)[split - 1], (node->children - 1) * _var_size);
        memmove(&sub(node)[1], &sub(node)[split], (node->children - 1) * sizeof(off_t));

        return split_key;
}

template<typename key_t>
key_t BplusTree<key_t>::non_leaf_split_right1( bplus_tree *tree,  bplus_node *node,
                        	    bplus_node *right,  bplus_node *l_ch,
                        	    bplus_node *r_ch, key_t key, int insert)
{
        int i;

        /* split = [m/2] */
        int split = (_max_order + 1) / 2;

        /* split as right sibling */
        right_node_add(tree, node, right);

        /* split key is key[split - 1] */
        key_t split_key = key(node)[split - 1];

        /* calculate split nodes' children (sum as (order + 1))*/
        int pivot = 0;
        node->children = split;
        right->children = _max_order - split + 1;

        /* insert new key and sub-nodes */
        key(right)[0] = key;
        sub_node_update(tree, right, pivot, l_ch);
        sub_node_update(tree, right, pivot + 1, r_ch);

        /* sum = right->children = 2 + (right->children - 2) */
        /* replicate from key[split] to key[_max_order - 2] */
        memmove(&key(right)[pivot + 1], &key(node)[split], (right->children - 2) * _var_size);
        memmove(&sub(right)[pivot + 2], &sub(node)[split + 1], (right->children - 2) * sizeof(off_t));

        /* flush sub-nodes of the new splitted right node */
        for (i = pivot + 2; i < right->children; i++) {
                sub_node_flush(tree, right, sub(right)[i]);
        }

        return split_key;
}

template<typename key_t>
key_t BplusTree<key_t>::non_leaf_split_right2( bplus_tree *tree,  bplus_node *node,
                        	    bplus_node *right,  bplus_node *l_ch,
                        	    bplus_node *r_ch, key_t key, int insert)
{
        int i;

        /* split = [m/2] */
        int split = (_max_order + 1) / 2;

        /* split as right sibling */
        right_node_add(tree, node, right);

        /* split key is key[split] */
        key_t split_key = key(node)[split];

        /* calculate split nodes' children (sum as (order + 1))*/
        int pivot = insert - split - 1;
        node->children = split + 1;
        right->children = _max_order - split;

        /* sum = right->children = pivot + 2 + (_max_order - insert - 1) */
        /* replicate from key[split + 1] to key[insert] */
        memmove(&key(right)[0], &key(node)[split + 1], pivot * _var_size);
        memmove(&sub(right)[0], &sub(node)[split + 1], pivot * sizeof(off_t));

        /* insert new key and sub-node */
        key(right)[pivot] = key;
        sub_node_update(tree, right, pivot, l_ch);
        sub_node_update(tree, right, pivot + 1, r_ch);

        /* replicate from key[insert] to key[order - 1] */
        memmove(&key(right)[pivot + 1], &key(node)[insert], (_max_order - insert - 1) * _var_size);
        memmove(&sub(right)[pivot + 2], &sub(node)[insert + 1], (_max_order - insert - 1) * sizeof(off_t));

        /* flush sub-nodes of the new splitted right node */
        for (i = 0; i < right->children; i++) {
                if (i != pivot && i != pivot + 1) {
                        sub_node_flush(tree, right, sub(right)[i]);
                }
        }

        return split_key;
}

template<typename key_t>
void BplusTree<key_t>::non_leaf_simple_insert( bplus_tree *tree,  bplus_node *node,
                        	    bplus_node *l_ch,  bplus_node *r_ch,
                        	   key_t key, int insert)
{
        memmove(&key(node)[insert + 1], &key(node)[insert], (node->children - 1 - insert) * _var_size);
        memmove(&sub(node)[insert + 2], &sub(node)[insert + 1], (node->children - 1 - insert) * sizeof(off_t));
        /* insert new key and sub-nodes */
        key(node)[insert] = key;
        sub_node_update(tree, node, insert, l_ch);
        sub_node_update(tree, node, insert + 1, r_ch);
        node->children++;
}

template<typename key_t>
int BplusTree<key_t>::non_leaf_insert( bplus_tree *tree,  bplus_node *node,
                	    bplus_node *l_ch,  bplus_node *r_ch, key_t key)
{
        /* Search key location */
        int insert = key_binary_search(node, key);
        assert(insert < 0);
        insert = -insert - 1;

        /* node is full */
        if (node->children == _max_order) {
                key_t split_key;
                /* split = [m/2] */
                int split = (node->children + 1) / 2;
                 bplus_node *sibling = non_leaf_new(tree);
                if (insert < split) {
                        split_key = non_leaf_split_left(tree, node, sibling, l_ch, r_ch, key, insert);
                } else if (insert == split) {
                        split_key = non_leaf_split_right1(tree, node, sibling, l_ch, r_ch, key, insert);
                } else {
                        split_key = non_leaf_split_right2(tree, node, sibling, l_ch, r_ch, key, insert);
                }

                /* build new parent */
                if (insert < split) {
                        return parent_node_build(tree, sibling, node, split_key);
                } else {
                        return parent_node_build(tree, node, sibling, split_key);
                }
        } else {
                non_leaf_simple_insert(tree, node, l_ch, r_ch, key, insert);
                node_flush(tree, node);
        }
        return 0;
}

template<typename key_t>
key_t BplusTree<key_t>::leaf_split_left( bplus_tree *tree,  bplus_node *leaf,
                	      bplus_node *left, key_t key, long long data, int insert)
{
        /* split = [m/2] */
        int split = (leaf->children + 1) / 2;

        /* split as left sibling */
        left_node_add(tree, leaf, left);

        /* calculate split leaves' children (sum as (entries + 1)) */
        int pivot = insert;
        left->children = split;
        leaf->children = _max_entries - split + 1;

        /* sum = left->children = pivot + 1 + (split - pivot - 1) */
        /* replicate from key[0] to key[insert] */
        memmove(&key(left)[0], &key(leaf)[0], pivot * _var_size);
        memmove(&data(left)[0], &data(leaf)[0], pivot * sizeof(long long));

        /* insert new key and data */
        key(left)[pivot] = key;
        data(left)[pivot] = data;

        /* replicate from key[insert] to key[split - 1] */
        memmove(&key(left)[pivot + 1], &key(leaf)[pivot], (split - pivot - 1) * _var_size);
        memmove(&data(left)[pivot + 1], &data(leaf)[pivot], (split - pivot - 1) * sizeof(long long));

        /* original leaf left shift */
        memmove(&key(leaf)[0], &key(leaf)[split - 1], leaf->children * _var_size);
        memmove(&data(leaf)[0], &data(leaf)[split - 1], leaf->children * sizeof(long long));

        return key(leaf)[0];
}

template<typename key_t>
key_t BplusTree<key_t>::leaf_split_right( bplus_tree *tree,  bplus_node *leaf,
                	       bplus_node *right, key_t key, long long data, int insert)
{
        /* split = [m/2] */
        int split = (leaf->children + 1) / 2;

        /* split as right sibling */
        right_node_add(tree, leaf, right);

        /* calculate split leaves' children (sum as (entries + 1)) */
        int pivot = insert - split;
        leaf->children = split;
        right->children = _max_entries - split + 1;

        /* sum = right->children = pivot + 1 + (_max_entries - pivot - split) */
        /* replicate from key[split] to key[children - 1] in original leaf */
        memmove(&key(right)[0], &key(leaf)[split], pivot * _var_size);
        memmove(&data(right)[0], &data(leaf)[split], pivot * sizeof(long long));

        /* insert new key and data */
        key(right)[pivot] = key;
        data(right)[pivot] = data;

        /* replicate from key[insert] to key[children - 1] in original leaf */
        memmove(&key(right)[pivot + 1], &key(leaf)[insert], (_max_entries - insert) * _var_size);
        memmove(&data(right)[pivot + 1], &data(leaf)[insert], (_max_entries - insert) * sizeof(long long));

        return key(right)[0];
}

template<typename key_t>
void BplusTree<key_t>::leaf_simple_insert( bplus_tree *tree,  bplus_node *leaf,
                	       key_t key, long long data, int insert)
{
        memmove(&key(leaf)[insert + 1], &key(leaf)[insert], (leaf->children - insert) * _var_size);
        memmove(&data(leaf)[insert + 1], &data(leaf)[insert], (leaf->children - insert) * sizeof(long long));
        key(leaf)[insert] = key;
        data(leaf)[insert] = data;
        leaf->children++;
}

template<typename key_t>
int BplusTree<key_t>::leaf_insert( bplus_tree *tree,  bplus_node *leaf, key_t key, long long data)
{
        /* Search key location */
        int insert = key_binary_search(leaf, key);
        if (insert >= 0) {
                /* Already exists */
                return -1;
        }
        insert = -insert - 1;

        /* fetch from free node caches */
        int i = ((char *) leaf - tree->caches) / _block_size;
        tree->used[i] = 1;

        /* leaf is full */
        if (leaf->children == _max_entries) {
                key_t split_key;
                /* split = [m/2] */
                int split = (_max_entries + 1) / 2;
                 bplus_node *sibling = leaf_new(tree);

                /* sibling leaf replication due to location of insertion */
                if (insert < split) {
                        split_key = leaf_split_left(tree, leaf, sibling, key, data, insert);
                } else {
                        split_key = leaf_split_right(tree, leaf, sibling, key, data, insert);
                }

                /* build new parent */
                if (insert < split) {
                        return parent_node_build(tree, sibling, leaf, split_key);
                } else {
                        return parent_node_build(tree, leaf, sibling, split_key);
                }
        } else {
                leaf_simple_insert(tree, leaf, key, data, insert);
                node_flush(tree, leaf);
        }

        return 0;
}

template<typename key_t>
int BplusTree<key_t>::bplus_tree_insert( bplus_tree *tree, key_t key, long long data)
{
         bplus_node *node = node_seek(tree, tree->root);
        while (node != NULL) {
                if (is_leaf(node)) {
                        return leaf_insert(tree, node, key, data);
                } else {
                        int i = key_binary_search(node, key);
                        if (i >= 0) {
                                node = node_seek(tree, sub(node)[i + 1]);
                        } else {
                                i = -i - 1;
                                node = node_seek(tree, sub(node)[i]);
                        }
                }
        }

        /* new root */
        bplus_node *root = leaf_new(tree);
        key(root)[0] = key;
        data(root)[0] = data;
        root->children = 1;
        tree->root = new_node_append(tree, root);
        tree->level = 1;
        node_flush(tree, root);
        return 0;
}

template<typename key_t>
int BplusTree<key_t>::sibling_select( bplus_node *l_sib,  bplus_node *r_sib,
                                  bplus_node *parent, int i)
{
        if (i == -1) {
                /* the frist sub-node, no left sibling, choose the right one */
                return RIGHT_SIBLING;
        } else if (i == parent->children - 2) {
                /* the last sub-node, no right sibling, choose the left one */
                return LEFT_SIBLING;
        } else {
                /* if both left and right sibling found, choose the one with more children */
                return l_sib->children >= r_sib->children ? LEFT_SIBLING : RIGHT_SIBLING;
        }
}

template<typename key_t>
void BplusTree<key_t>::non_leaf_shift_from_left( bplus_tree *tree,  bplus_node *node,
                        	      bplus_node *left,  bplus_node *parent,
                        	     int parent_key_index, int remove)
{
        /* node's elements right shift */
        memmove(&key(node)[1], &key(node)[0], remove * _var_size);
        memmove(&sub(node)[1], &sub(node)[0], (remove + 1) * sizeof(off_t));

        /* parent key right rotation */
        key(node)[0] = key(parent)[parent_key_index];
        key(parent)[parent_key_index] = key(left)[left->children - 2];

        /* borrow the last sub-node from left sibling */
        sub(node)[0] = sub(left)[left->children - 1];
        sub_node_flush(tree, node, sub(node)[0]);

        left->children--;
}

template<typename key_t>
void BplusTree<key_t>::non_leaf_merge_into_left( bplus_tree *tree,  bplus_node *node,
                        	      bplus_node *left,  bplus_node *parent,
                        	     int parent_key_index, int remove)
{
        /* move parent key down */
        key(left)[left->children - 1] = key(parent)[parent_key_index];

        /* merge into left sibling */
        /* key sum = node->children - 2 */
        memmove(&key(left)[left->children], &key(node)[0], remove * _var_size);
        memmove(&sub(left)[left->children], &sub(node)[0], (remove + 1) * sizeof(off_t));

        /* sub-node sum = node->children - 1 */
        memmove(&key(left)[left->children + remove], &key(node)[remove + 1], (node->children - remove - 2) * _var_size);
        memmove(&sub(left)[left->children + remove + 1], &sub(node)[remove + 2], (node->children - remove - 2) * sizeof(off_t));

        /* flush sub-nodes of the new merged left node */
        int i, j;
        for (i = left->children, j = 0; j < node->children - 1; i++, j++) {
                sub_node_flush(tree, left, sub(left)[i]);
        }

        left->children += node->children - 1;
}

template<typename key_t>
void BplusTree<key_t>::non_leaf_shift_from_right( bplus_tree *tree,  bplus_node *node,
                        	       bplus_node *right,  bplus_node *parent,
                        	      int parent_key_index)
{
        /* parent key left rotation */
        key(node)[node->children - 1] = key(parent)[parent_key_index];
        key(parent)[parent_key_index] = key(right)[0];

        /* borrow the frist sub-node from right sibling */
        sub(node)[node->children] = sub(right)[0];
        sub_node_flush(tree, node, sub(node)[node->children]);
        node->children++;

        /* right sibling left shift*/
        memmove(&key(right)[0], &key(right)[1], (right->children - 2) * _var_size);
        memmove(&sub(right)[0], &sub(right)[1], (right->children - 1) * sizeof(off_t));

        right->children--;
}

template<typename key_t>
void BplusTree<key_t>::non_leaf_merge_from_right( bplus_tree *tree,  bplus_node *node,
                        	       bplus_node *right,  bplus_node *parent,
                        	      int parent_key_index)
{
        /* move parent key down */
        key(node)[node->children - 1] = key(parent)[parent_key_index];
        node->children++;

        /* merge from right sibling */
        memmove(&key(node)[node->children - 1], &key(right)[0], (right->children - 1) * _var_size);
        memmove(&sub(node)[node->children - 1], &sub(right)[0], right->children * sizeof(off_t));

        /* flush sub-nodes of the new merged node */
        int i, j;
        for (i = node->children - 1, j = 0; j < right->children; i++, j++) {
                sub_node_flush(tree, node, sub(node)[i]);
        }

        node->children += right->children - 1;
}

template<typename key_t>
void BplusTree<key_t>::non_leaf_simple_remove( bplus_tree *tree,  bplus_node *node, int remove)
{
        assert(node->children >= 2);
        memmove(&key(node)[remove], &key(node)[remove + 1], (node->children - remove - 2) * _var_size);
        memmove(&sub(node)[remove + 1], &sub(node)[remove + 2], (node->children - remove - 2) * sizeof(off_t));
        node->children--;
}

template<typename key_t>
void BplusTree<key_t>::non_leaf_remove( bplus_tree *tree,  bplus_node *node, int remove)
{
        if (node->parent == INVALID_OFFSET) {
                /* node is the root */
                if (node->children == 2) {
                        /* replace old root with the first sub-node */
                         bplus_node *root = node_fetch(tree, sub(node)[0]);
                        root->parent = INVALID_OFFSET;
                        tree->root = root->self;
                        tree->level--;
                        node_delete(tree, node, NULL, NULL);
                        node_flush(tree, root);
                } else {
                        non_leaf_simple_remove(tree, node, remove);
                        node_flush(tree, node);
                }
        } else if (node->children <= (_max_order + 1) / 2) {
                 bplus_node *l_sib = node_fetch(tree, node->prev);
                 bplus_node *r_sib = node_fetch(tree, node->next);
                 bplus_node *parent = node_fetch(tree, node->parent);

                int i = parent_key_index(parent, key(node)[0]);

                /* decide which sibling to be borrowed from */
                if (sibling_select(l_sib, r_sib, parent, i)  == LEFT_SIBLING) {
                        if (l_sib->children > (_max_order + 1) / 2) {
                                non_leaf_shift_from_left(tree, node, l_sib, parent, i, remove);
                                /* flush nodes */
                                node_flush(tree, node);
                                node_flush(tree, l_sib);
                                node_flush(tree, r_sib);
                                node_flush(tree, parent);
                        } else {
                                non_leaf_merge_into_left(tree, node, l_sib, parent, i, remove);
                                /* delete empty node and flush */
                                node_delete(tree, node, l_sib, r_sib);
                                /* trace upwards */
                                non_leaf_remove(tree, parent, i);
                        }
                } else {
                        /* remove at first in case of overflow during merging with sibling */
                        non_leaf_simple_remove(tree, node, remove);

                        if (r_sib->children > (_max_order + 1) / 2) {
                                non_leaf_shift_from_right(tree, node, r_sib, parent, i + 1);
                                /* flush nodes */
                                node_flush(tree, node);
                                node_flush(tree, l_sib);
                                node_flush(tree, r_sib);
                                node_flush(tree, parent);
                        } else {
                                non_leaf_merge_from_right(tree, node, r_sib, parent, i + 1);
                                /* delete empty right sibling and flush */
                                 bplus_node *rr_sib = node_fetch(tree, r_sib->next);
                                node_delete(tree, r_sib, node, rr_sib);
                                node_flush(tree, l_sib);
                                /* trace upwards */
                                non_leaf_remove(tree, parent, i + 1);
                        }
                }
        } else {
                non_leaf_simple_remove(tree, node, remove);
                node_flush(tree, node);
        }
}

template<typename key_t>
void BplusTree<key_t>::leaf_shift_from_left( bplus_tree *tree,  bplus_node *leaf,
                		  bplus_node *left,  bplus_node *parent,
                		 int parent_key_index, int remove)
{
        /* right shift in leaf node */
        memmove(&key(leaf)[1], &key(leaf)[0], remove * _var_size);
        memmove(&data(leaf)[1], &data(leaf)[0], remove * sizeof(off_t));

        /* borrow the last element from left sibling */
        key(leaf)[0] = key(left)[left->children - 1];
        data(leaf)[0] = data(left)[left->children - 1];
        left->children--;

        /* update parent key */
        key(parent)[parent_key_index] = key(leaf)[0];
}

template<typename key_t>
void BplusTree<key_t>::leaf_merge_into_left( bplus_tree *tree,  bplus_node *leaf,
                		  bplus_node *left, int parent_key_index, int remove)
{
        /* merge into left sibling, sum = leaf->children - 1*/
        memmove(&key(left)[left->children], &key(leaf)[0], remove * _var_size);
        memmove(&data(left)[left->children], &data(leaf)[0], remove * sizeof(off_t));
        memmove(&key(left)[left->children + remove], &key(leaf)[remove + 1], (leaf->children - remove - 1) * _var_size);
        memmove(&data(left)[left->children + remove], &data(leaf)[remove + 1], (leaf->children - remove - 1) * sizeof(off_t));
        left->children += leaf->children - 1;
}

template<typename key_t>
void BplusTree<key_t>::leaf_shift_from_right( bplus_tree *tree,  bplus_node *leaf,
                                   bplus_node *right,  bplus_node *parent,
                                  int parent_key_index)
{
        /* borrow the first element from right sibling */
        key(leaf)[leaf->children] = key(right)[0];
        data(leaf)[leaf->children] = data(right)[0];
        leaf->children++;

        /* left shift in right sibling */
        memmove(&key(right)[0], &key(right)[1], (right->children - 1) * _var_size);
        memmove(&data(right)[0], &data(right)[1], (right->children - 1) * sizeof(off_t));
        right->children--;

        /* update parent key */
        key(parent)[parent_key_index] = key(right)[0];
}

template<typename key_t>
void BplusTree<key_t>::leaf_merge_from_right( bplus_tree *tree,  bplus_node *leaf,
                                          bplus_node *right)
{
        memmove(&key(leaf)[leaf->children], &key(right)[0], right->children * _var_size);
        memmove(&data(leaf)[leaf->children], &data(right)[0], right->children * sizeof(off_t));
        leaf->children += right->children;
}

template<typename key_t>
void BplusTree<key_t>::leaf_simple_remove( bplus_tree *tree,  bplus_node *leaf, int remove)
{
        memmove(&key(leaf)[remove], &key(leaf)[remove + 1], (leaf->children - remove - 1) * _var_size);
        memmove(&data(leaf)[remove], &data(leaf)[remove + 1], (leaf->children - remove - 1) * sizeof(off_t));
        leaf->children--;
}

template<typename key_t>
int BplusTree<key_t>::leaf_remove( bplus_tree *tree,  bplus_node *leaf, key_t key)
{
        int remove = key_binary_search(leaf, key);
        if (remove < 0) {
                /* Not exist */
                return -1;
        }

        /* fetch from free node caches */
        int i = ((char *) leaf - tree->caches) / _block_size;
        tree->used[i] = 1;

        if (leaf->parent == INVALID_OFFSET) {
                /* leaf as the root */
                if (leaf->children == 1) {
                        /* delete the only last node */
                        assert(key == key(leaf)[0]);
                        tree->root = INVALID_OFFSET;
                        tree->level = 0;
                        node_delete(tree, leaf, NULL, NULL);
                } else {
                        leaf_simple_remove(tree, leaf, remove);
                        node_flush(tree, leaf);
                }
        } else if (leaf->children <= (_max_entries + 1) / 2) {
                 bplus_node *l_sib = node_fetch(tree, leaf->prev);
                 bplus_node *r_sib = node_fetch(tree, leaf->next);
                 bplus_node *parent = node_fetch(tree, leaf->parent);

                i = parent_key_index(parent, key(leaf)[0]);

                /* decide which sibling to be borrowed from */
                if (sibling_select(l_sib, r_sib, parent, i) == LEFT_SIBLING) {
                        if (l_sib->children > (_max_entries + 1) / 2) {
                                leaf_shift_from_left(tree, leaf, l_sib, parent, i, remove);
                                /* flush leaves */
                                node_flush(tree, leaf);
                                node_flush(tree, l_sib);
                                node_flush(tree, r_sib);
                                node_flush(tree, parent);
                        } else {
                                leaf_merge_into_left(tree, leaf, l_sib, i, remove);
                                /* delete empty leaf and flush */
                                node_delete(tree, leaf, l_sib, r_sib);
                                /* trace upwards */
                                non_leaf_remove(tree, parent, i);
                        }
                } else {
                        /* remove at first in case of overflow during merging with sibling */
                        leaf_simple_remove(tree, leaf, remove);

                        if (r_sib->children > (_max_entries + 1) / 2) {
                                leaf_shift_from_right(tree, leaf, r_sib, parent, i + 1);
                                /* flush leaves */
                                node_flush(tree, leaf);
                                node_flush(tree, l_sib);
                                node_flush(tree, r_sib);
                                node_flush(tree, parent);
                        } else {
                                leaf_merge_from_right(tree, leaf, r_sib);
                                /* delete empty right sibling flush */
                                 bplus_node *rr_sib = node_fetch(tree, r_sib->next);
                                node_delete(tree, r_sib, leaf, rr_sib);
                                node_flush(tree, l_sib);
                                /* trace upwards */
                                non_leaf_remove(tree, parent, i + 1);
                        }
                }
        } else {
                leaf_simple_remove(tree, leaf, remove);
                node_flush(tree, leaf);
        }

        return 0;
}

template<typename key_t>
int BplusTree<key_t>::bplus_tree_delete( bplus_tree *tree, key_t key)
{
        bplus_node *node = node_seek(tree, tree->root);
        while (node != NULL) {
                if (is_leaf(node)) {
                        return leaf_remove(tree, node, key);
                } else {
                        int i = key_binary_search(node, key);
                        if (i >= 0) {
                                node = node_seek(tree, sub(node)[i + 1]);
                        } else {
                                i = -i - 1;
                                node = node_seek(tree, sub(node)[i]);
                        }
                }
        }
        return -1;
}

template<typename key_t>
long long BplusTree<key_t>::bplus_tree_get( bplus_tree *tree, key_t key)
{
        return bplus_tree_search(tree, key);
}

template<typename key_t>
int BplusTree<key_t>::bplus_tree_put( bplus_tree *tree, key_t key, long long data)
{
        if (data) {
                return bplus_tree_insert(tree, key, data);
        } else {
                return bplus_tree_delete(tree, key);
        }
}

// offset指的是第几条记录
void recordFilePtrForIndex(Pointer ptr, int offset, BufferManager &TM);

// key1表示作为判断依据的值
// beqmin：1表示大于，0表示大于等于
// beqmax：1表示小于，0表示小于等于
// pivot：1表示取左边，0表示取右边
// 如果beqmin和beqmax的值都是1，那么表示不等于key1
template<typename key_t>
int BplusTree<key_t>::bplus_tree_get_range(bplus_tree *tree, char *filename, key_t key1, int beqmin, int beqmax, int pivot, BufferManager &TM)
{
        // cout << all_min << " " << all_max << endl;
        if(beqmin == 1 && beqmax == 1 && pivot == 1) {
                long long data = bplus_tree_get(tree, key1);
                if(data == -1){
                        // cout << "No such key" << endl;
                        return 0;
                }         
                int blockid;
                int offset;
                blockid = data >> 32;
                offset = data & 0xFFFFFFFF;
                recordFilePtrForIndex(Pointer(blockid, offset), 0, TM);
                return 1;
        }
        else{
                long long start = -1;
                int val_num = 0;
                long long neqdata = 0;
                int file_path = bplus_open(filename);
                key_t all_max;
                key_t all_min;
                key_t min;
                key_t max;

                bplus_node *node = node_seek(tree, tree->root);
                bplus_node *tempNode = node;

                while(!is_leaf(tempNode)){
                        tempNode = node_seek(tree, sub(node)[0]);
                }
                all_min = key(tempNode)[0];
                // cout << "all_min: " << all_min << endl;
                // cout << "data(tempNode)[0]S: " << key(tempNode)[0] << endl;
                while (node_seek(tree, tempNode->next) != NULL) {
                        // cout << "data(tempNode)[0]" << key(tempNode)[0] << endl;
                        // tempNode = node_seek(tree, tempNode->next);
                }
                int index = 0;
                // cout << "data(tempNode)[index]" << data(tempNode)[index] << endl;
                while(data(tempNode)[index] != 0){
                        index++;
                }
                all_max = key(tempNode)[index - 1];
                // cout << "all_max: " << all_max << endl;
                if(beqmin == 1 && beqmax == 1 && pivot == 0){
                        neqdata = bplus_tree_get(tree, key1);
                        // cout <<  "neqdata: " << neqdata << endl;
                        min = all_min;
                        max = all_max;
                        beqmin = 0;
                        beqmax = 0;
                }else if(key1 < all_min && pivot == 0){
                        min = all_min;
                        max = all_max;
                        beqmin = 0;
                        beqmax = 0;
                }else if(key1 > all_max && pivot == 1){
                        min = all_min;
                        max = all_max;
                        beqmin = 0;
                        beqmax = 0;
                }
                else if((key1 == all_min && beqmax == 1) || (key1 == all_max && beqmin == 1) ){
                        return 0;
                }
                else if(key1 >= all_min && key1 <= all_max && pivot == 0){
                        if(beqmin == 1){
                                neqdata = bplus_tree_get(tree, key1);
                        }
                        min = key1;
                        max = all_max;
                }else if(key1 >= all_min && key1 <= all_max && pivot == 1){
                        min = all_min;
                        max = key1;
                        if(beqmax == 1){
                                neqdata = bplus_tree_get(tree, key1);
                        }
                }else{
                        // 没有符合搜索条件的值
                        return 0;
                }
                
                node = node_seek(tree, tree->root);
                // cout << "min: " << min << " max: " << max << endl; 
                while (node != NULL) {
                        int i = key_binary_search(node, min);
                        // cout << "i: " << i << endl;
                        if (is_leaf(node)) {
                                if (i < 0) {
                                        i = -i - 1;
                                        if (i >= node->children) {
                                                node = node_seek(tree, node->next);
                                        }
                                }
                                while (node != NULL && key(node)[i] <= max) {
                                        start = data(node)[i];
                                        // cout << "data: " << start << " key: " << key(node)[i]<< endl;
                                        if(start != neqdata){
                                                // cout << "neq data: " << start << endl;
                                                long long temp;
                                                int blockid;
                                                int offset;
                                                blockid = start >> 32;
                                                offset = start & 0xFFFFFFFF;
                                                // write(file_path, &blockid, sizeof(int));
                                                // write(file_path, &offset, sizeof(int));
                                                recordFilePtrForIndex(Pointer(blockid, offset), val_num, TM);
                                                val_num++;
                                        }
                                        if (++i >= node->children) {
                                                node = node_seek(tree, node->next);
                                                i = 0;
                                        }
                                }
                                break;
                        } else {
                                if (i >= 0) {
                                        node = node_seek(tree, sub(node)[i + 1]);
                                } else  {
                                        i = -i - 1;
                                        node = node_seek(tree, sub(node)[i]);
                                }
                        }
                }
                bplus_close(file_path);

                return val_num;
        }
}

template<typename key_t>
int BplusTree<key_t>::bplus_open(char *filename)
{
        return open(filename, O_CREAT | O_RDWR, 0644);
}

template<typename key_t>
void BplusTree<key_t>::bplus_close(int fd)
{
        close(fd);
}

template<typename key_t>
off_t BplusTree<key_t>::str_to_hex(char *c, int len)
{
        off_t offset = 0;
        while (len-- > 0) {
                if (isdigit(*c)) {
                        offset = offset * 16 + *c - '0';
                } else if (isxdigit(*c)) {
                        if (islower(*c)) {
                                offset = offset * 16 + *c - 'a' + 10;
                        } else {
                                offset = offset * 16 + *c - 'A' + 10;
                        }
                }
                c++;
        }
        return offset;
}

template<typename key_t>
void BplusTree<key_t>::hex_to_str(off_t offset, char *buf, int len)
{
        const static char *hex = "0123456789ABCDEF";
        while (len-- > 0) {
                buf[len] = hex[offset & 0xf];
                offset >>= 4;
        }
}

template<typename key_t>
off_t BplusTree<key_t>::offset_load(int fd)
{
        char buf[ADDR_STR_WIDTH];
        ssize_t len = read(fd, buf, sizeof(buf));
        return len > 0 ? str_to_hex(buf, sizeof(buf)) : INVALID_OFFSET;
}

template<typename key_t>
ssize_t BplusTree<key_t>::offset_store(int fd, off_t offset)
{
        char buf[ADDR_STR_WIDTH];
        hex_to_str(offset, buf, sizeof(buf));
        return write(fd, buf, sizeof(buf));
}

template<typename key_t>
bplus_tree * BplusTree<key_t>::bplus_tree_init(char *filename, int block_size)
{
        int i;
        bplus_node node;

        if (strlen(filename) >= 1024) {
                fprintf(stderr, "Index file name too long long!\n");
                return NULL;
        }

        if ((block_size & (block_size - 1)) != 0) {
                fprintf(stderr, "Block size must be pow of 2!\n");
                return NULL;
        }

        if (block_size < (int) sizeof(node)) {
                fprintf(stderr, "block size is too small for one node!\n");
                return NULL;
        }

        _block_size = block_size;
        _max_order = (block_size - sizeof(node)) / (_var_size + sizeof(off_t));
        _max_entries = (block_size - sizeof(node)) / (_var_size + sizeof(long long));
        if (_max_order <= 2) {
                fprintf(stderr, "block size is too small for one node!\n");
                return NULL;
        }

        bplus_tree *tree = (bplus_tree *)calloc(1, sizeof(*tree));
        assert(tree != NULL);
        list_init(&tree->free_blocks);
        strcpy(tree->filename, filename);

        /* load index boot file */
        int fd = open(strcat(tree->filename, ".boot"), O_RDWR, 0644);
        if (fd >= 0) {
                tree->root = offset_load(fd);
                _block_size = offset_load(fd);
                tree->file_size = offset_load(fd);
                /* load free blocks */
                while ((i = offset_load(fd)) != INVALID_OFFSET) {
                        free_block *block = (free_block *)malloc(sizeof(*block));
                        assert(block != NULL);
                        block->offset = i;
                        list_add(&block->link, &tree->free_blocks);
                }
                close(fd);
        } else {
                tree->root = INVALID_OFFSET;
                _block_size = block_size;
                tree->file_size = 0;
        }

        /* set order and entries */
        _max_order = (_block_size - sizeof(node)) / (_var_size + sizeof(off_t));
        _max_entries = (_block_size - sizeof(node)) / (_var_size + sizeof(long long));
        //printf("config node order:%d and leaf entries:%d\n", _max_order, _max_entries);

        /* init free node caches */
        tree->caches = (char *)malloc(_block_size * MIN_CACHE_NUM);

        /* open data file */
        tree->fd = bplus_open(filename);
        assert(tree->fd >= 0);
        return tree;
}

template<typename key_t>
void BplusTree<key_t>::bplus_tree_deinit( bplus_tree *tree)
{
        int fd = open(tree->filename, O_CREAT | O_RDWR, 0644);
        assert(fd >= 0);
        assert(offset_store(fd, tree->root) == ADDR_STR_WIDTH);
        assert(offset_store(fd, _block_size) == ADDR_STR_WIDTH);
        assert(offset_store(fd, tree->file_size) == ADDR_STR_WIDTH);

        /* store free blocks in files for future reuse */
         list_head *pos, *n;
        list_for_each_safe(pos, n, &tree->free_blocks) {
                list_del(pos);
                 free_block *block = list_entry(pos,  free_block, link);
                assert(offset_store(fd, block->offset) == ADDR_STR_WIDTH);
                free(block);
        }

        bplus_close(tree->fd);
        free(tree->caches);
        free(tree);
}

// pivot：大于0返回一个恰好比key大的值，小于0返回一个最小的值
template<typename key_t>
long long BplusTree<key_t>::bplus_tree_get_con(bplus_tree *tree, key_t key1, int pivot)
{
        long long ret = -1;
        long long ret_min = -1;
        key_t all_min;
        key_t all_max;

        bplus_node *node = node_seek(tree, tree->root);
        bplus_node *tempNode = node;

        while(!is_leaf(tempNode)){
                tempNode = node_seek(tree, sub(node)[0]);
        }
        all_min = key(tempNode)[0];
        ret_min = data(tempNode)[0];
        node = tempNode;
        while (node_seek(tree, tempNode->next) != NULL) {
                tempNode = node_seek(tree, tempNode->next);
        }
        int index = 0;
        while(data(tempNode)[index] != 0){
                index++;
        }
        all_max = key(tempNode)[index - 1];
        if(key1 < all_min || key1 > all_max){
                return -1;
        }

        if(pivot < 0){
                return ret_min;
        }
        ret = data(tempNode)[index - 1];
        return ret;       
}

// 取消宏定义
#undef MIN_CACHE_NUM
#undef ADDR_STR_WIDTH
#undef list_entry//(ptr, type, member)
#undef list_first_entry//(ptr, type, member)
#undef list_last_entry//(ptr, type, member)
#undef list_for_each//(pos, head)
#undef list_for_each_safe//(pos, n, head)
#undef offset_ptr//(node)
#undef key//(node)
#undef data//(node)
#undef sub//(node)

#endif