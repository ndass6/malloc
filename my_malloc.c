#include "my_malloc.h"

/* You *MUST* use this macro when calling my_sbrk to allocate the
 * appropriate size. Failure to do so may result in an incorrect
 * grading!
 */
#define SBRK_SIZE 2048

/* If you want to use debugging printouts, it is HIGHLY recommended
 * to use this macro or something similar. If you produce output from
 * your code then you may receive a 20 point deduction. You have been
 * warned.
 */
#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x)
#endif


/* our freelist structure - this is where the current freelist of
 * blocks will be maintained. failure to maintain the list inside
 * of this structure will result in no credit, as the grader will
 * expect it to be maintained here.
 * Technically this should be declared static for the same reasons
 * as above, but DO NOT CHANGE the way this structure is declared
 * or it will break the autograder.
 */
metadata_t* freelist;

void add_to_free_list_size(metadata_t* newNode);
void add_to_free_list_address(metadata_t* newNode);
metadata_t* split_memory_size(size_t size);
metadata_t* split_memory_address(size_t size);
metadata_t* find_node_size(size_t size);
metadata_t* find_node_address(size_t size);
metadata_t* remove_from_free_list(metadata_t* curr);

void* my_malloc_size_order(size_t size) {
  // Make sure size is greater than 0
  if (size <= 0) {
  	return NULL;
  }

  // Spaced needed including metadata
  size_t space = size + sizeof(metadata_t);

  // Checks if user requests too much space
  if (space > SBRK_SIZE) {
  	ERRNO = SINGLE_REQUEST_TOO_LARGE;
  	return NULL;
  }

  metadata_t* currentBlock = split_memory_size(size);

  // Check if the block exists in memory
  if (currentBlock) {
    return currentBlock + 1;
  } else {
    // Allocate space
    metadata_t* heap = my_sbrk(SBRK_SIZE);

    /// Out of memory
    if (!heap) {
      ERRNO = OUT_OF_MEMORY;
      return NULL;
    }

    (*heap).in_use = 0;
    (*heap).size = SBRK_SIZE;
    (*heap).next = NULL;
    (*heap).prev = NULL;

    // Add the new block to the free list
    add_to_free_list_size(heap);

    // Split the block
    currentBlock = split_memory_size(size);
    ERRNO = NO_ERROR;
    return currentBlock + 1;
  }
  return NULL;
}

void* my_malloc_addr_order(size_t size) {
  // Make sure size is greater than 0
  if (size <= 0) {
    return NULL;
  }

  // Space needed including metadata
  size_t space = size + sizeof(metadata_t);

  // Checks if user requests too much space
  if (space > SBRK_SIZE) {
    ERRNO = SINGLE_REQUEST_TOO_LARGE;
    return NULL;
  }

  metadata_t* currentBlock = split_memory_address(size);

  // Check if the blox exists in memory
  if (currentBlock) {
    return currentBlock + 1;
  } else {
    // Alleocate space
    metadata_t* heap = my_sbrk(SBRK_SIZE);

    // Out of memory
    if (!heap) {
      ERRNO = OUT_OF_MEMORY;
      return NULL;
    }

    (*heap).in_use = 0;
    (*heap).size = SBRK_SIZE;
    (*heap).next = NULL;
    (*heap).prev = NULL;

    // Add the new block to the free list
    add_to_free_list_address(heap);
 
    // Split the block
    currentBlock = split_memory_address(size);
    ERRNO = NO_ERROR;
    return currentBlock + 1;
  }
  return NULL;
}

void my_free_size_order(void* ptr) {
  // Make sure the pointer is not null
  if (!ptr) {
    return;
  }

  // Offset the pointer
  metadata_t* block = (metadata_t*) ((char*) ptr - sizeof(metadata_t));
  (*block).in_use = 0;


  // Make sure the block is not null
  if (block) {
    // Add the block to the free list
    add_to_free_list_size(block);

    // The left block is located before an offset of this block's size
    metadata_t* left = freelist;
    while ((metadata_t*) ((char*) left + (*left).size) != block) {
      left = (*left).next;
      if (!left) {
        left = NULL;
        break;
      }
    }
    
    // The right block is located after an offset of this block's size
    metadata_t* right = freelist;
    while ((metadata_t*) ((char*) block + (*block).size) != right) {
      right = (*right).next;
      if (!right) {
        right = NULL;
        break;
      }
    }

    // If the left block is not null or being used
    if (left && !(*left).in_use) {
      // Remove both blocks from the free list
      remove_from_free_list(block);
      remove_from_free_list(left);

      // Update the size of the new block
      (*left).size = (*left).size + (*block).size;

      // Add the new block to the free list
      add_to_free_list_size(left);
      block = left;
    }
     
    // If the right block is not null or being used
    if (right && !(*right).in_use) {
      // Remove both blocks from the free list
      remove_from_free_list(block);
      remove_from_free_list(right);
      
      // Update the size of the new block
      (*block).size = (*right).size + (*block).size;

      // Add the new block to the free list
      add_to_free_list_size(block);
    }
    ERRNO = NO_ERROR;
  }
}

void my_free_addr_order(void* ptr) {
  if (!ptr) {
    return;
  }

  // Offset the pointer
  metadata_t* block = (metadata_t*) ((char*) ptr - sizeof(metadata_t));
  (*block).in_use = 0;

  // Make sure block is not null
  if (block) {
    // Add the block to the free list
    add_to_free_list_address(block);

    // Left is the previous block if it is not null, not in use, and the addresses differ by the size of the current block
    metadata_t* left = NULL;
    if ((*block).prev && !(*(*block).prev).in_use && (metadata_t*) ((char*) (*block).prev + (*(*block).prev).size) == block) {
       left = (*block).prev;
    }
    
    // Right is the next block if it is not null, not in use, and the addresses differ by the size of the current block
    metadata_t* right = NULL;
    if ((*block).next && !(*(*block).next).in_use && (metadata_t*) ((char*) block + (*block).size) == (*block).next) {
      right = (*block).next;
    }

    // If the left block is not null or being used
    if (left && !(*left).in_use) {
      (*left).size = (*left).size + (*block).size;
      remove_from_free_list(block);
      block = left;
    }
    
    if (right && !(*right).in_use) {
      (*block).size = (*right).size + (*block).size;
      remove_from_free_list(right);
    }
    ERRNO = NO_ERROR;
  }
}

void add_to_free_list_size(metadata_t* newNode) {
  if (!newNode) {
    return;
  }
  if (!freelist) {
    freelist = newNode;
    return;
  }

  // Want to add the new node to the right of curr
  metadata_t* curr = find_node_size((*newNode).size);

  if ((*curr).next && (*curr).prev) {
    // Add before curr (curr is in the middle of the list)
    (*newNode).prev = (*curr).prev;
    (*newNode).next = curr;
    (*(*curr).prev).next = newNode;
    (*curr).prev = newNode;
  } else if (!(*curr).next && (*curr).prev) {
    // Add after curr (curr does not have a next)
    (*curr).next = newNode;
    (*newNode).prev = curr;
    (*newNode).next = NULL;
  } else if ((*curr).next && !(*curr).prev && (*curr).size < (*newNode).size) {
    // Add after curr (curr has a next)
    (*newNode).next = (*curr).next;
    (*(*curr).next).prev = newNode;
    (*newNode).prev = curr;
    (*curr).next = newNode;
  } else if ((*curr).next && !(*curr).prev && (*curr).size >= (*newNode).size) {
    // Add before curr (curr does not have a prev)
    (*newNode).next = curr;
    (*curr).prev = newNode;
    (*newNode).prev = NULL;
    freelist = newNode;
  } else if (curr == freelist && (*curr).size < (*newNode).size) {
    // Add after curr (curr equals the free list)
    (*curr).next = newNode;
    (*newNode).prev = curr;
    (*newNode).next = NULL;
  } else if (curr == freelist && (*curr).size >= (*newNode).size) {
    // Add before curr (curr equals the free list)
    (*curr).prev = newNode;
    (*newNode).next = curr;
    (*newNode).prev = NULL;
    freelist = newNode;
  }
}

void add_to_free_list_address(metadata_t* newNode) {
  // Make sure the new node is not null
  if (!newNode) {
    return;
  }

  // Check if free list is null
  if (!freelist) {
    freelist = newNode;
    return;
  }

  // We want to add the new node to the left of curr
  metadata_t* curr = freelist;
  while (curr < newNode && !((*curr).next >= newNode || !(*curr).next))
    curr = (*curr).next;

  if ((*curr).next && (*curr).prev) {
    // Add before curr (curr is in the middle of the free list)
    (*newNode).prev = curr;
    (*newNode).next = (*curr).next;
    (*(*curr).next).prev = newNode;
    (*curr).next = newNode;
  } else if (!(*curr).next && (*curr).prev) {
    // Add after curr (curr does not have a next)
    (*curr).next = newNode;
    (*newNode).prev = curr;
    (*newNode).next = NULL;
  } else if ((*curr).next && !(*curr).prev && (curr < newNode)) {
    // Add after curr (curr does not have a prev)
    (*newNode).next = (*curr).next;
    (*(*curr).next).prev = newNode;
    (*newNode).prev = curr;
    (*curr).next = newNode;
  } else if ((*curr).next && !(*curr).prev && (curr >= newNode)) {
    // Add before curr
    (*newNode).next = curr;
    (*curr).prev = newNode;
    (*newNode).prev = NULL;
    freelist = newNode;
  } else if (curr == freelist && (curr >= newNode)) {
    // Add to front of free list
    (*curr).prev = newNode;
    (*newNode).next = curr;
    (*newNode).prev = NULL;
    freelist = newNode;
  } else if (curr == freelist && (curr < newNode)) {
    // Add to end of free list
    (*curr).next = newNode;
    (*newNode).prev = curr;
    (*newNode).next = NULL;
  }
}

metadata_t* split_memory_size(size_t size) {
  // Make sure size is greater than 0
  if (size < 0)
    return NULL;

  // Make sure the free list is not null
  if (!freelist)
    return NULL;

  size_t space = (size_t) (size + sizeof(metadata_t));

  // Find the smallest necessary piece of memory
  metadata_t* curr = find_node_size(space);

  // Make sure that the size of curr is big enough
  if ((*curr).size < space)
    curr = (*curr).next;

  // Make sure curr is not null
  if (curr) {
    // Remove curr from the free list
    curr = remove_from_free_list(curr);
    metadata_t* newNode = curr;

    // Make sure curr is big enough to hold the metadata as well
    if ((*curr).size >= sizeof(metadata_t) + space + 1) {
      short newSize = (*curr).size - space;
      curr = (metadata_t*) ((char*) curr + space);
      (*curr).size = newSize;
      (*curr).in_use = 0;
      (*curr).next = NULL;
      (*curr).prev = NULL;

      // Add curr back to the free list
      add_to_free_list_size(curr);
    }
    (*newNode).size = (short) space;
    (*newNode).in_use = 1;
    (*newNode).next = NULL;
    (*newNode).prev = NULL;

    return newNode;
  }

  // Memory was not split
  return NULL;
}

metadata_t* split_memory_address(size_t size) {
  // Make sure size is greater than 0
  if (size < 0)
    return NULL;

  // Make sure the free list is not null
  if (!freelist)
    return NULL;

  size_t space = (size_t) (size + sizeof(metadata_t));

  // Find the smallest necessary piece of memory
  metadata_t* curr = find_node_address(space);

  // Make sure curr is not null
  if (curr) {
    // Remove curr from the free list
    curr = remove_from_free_list(curr);
    metadata_t* newNode = curr;

    // Make sure curr is big enough to hold the metadata as well
    if ((*curr).size >= space + sizeof(metadata_t) + 1) {
      short newSize = (*curr).size - space;
      curr = (metadata_t*) ((char*) curr + space);
      (*curr).size = newSize;
      (*curr).in_use = 0;
      (*curr).next = NULL;
      (*curr).prev = NULL;

      // Add curr back to the free list
      add_to_free_list_address(curr);
    }
    (*newNode).size = (short) space;
    (*newNode).in_use = 1;
    (*newNode).next = NULL;
    (*newNode).prev = NULL;

    return newNode;
  }

  // Memory was not split
  return NULL;
}

metadata_t* find_node_size(size_t size) {
  // Make sure size is greater than 0
  if (size < 0)
    return NULL;

  // Make sure free list is not null
  if (!freelist)
    return NULL;

  metadata_t* curr = freelist;
  while ((*curr).size < size && curr) {
    // We found the node if there is no next node or if the next node's
    // size is bigger than needed
    if (!(*curr).next || ((*curr).next && (*(*curr).next).size > size)) {
      return curr;
    }

    // We did not find the node, so go to the next node
    curr = (*curr).next;
  }
  return curr;
}

metadata_t* find_node_address(size_t size) {
  // Make sure size is greater than 0
  if (size < 0)
    return NULL;

  // If the free list is empty, return NULL
  if (!freelist)
    return NULL;

  // If there is only one elemnt in the free list, return the free list
  if (!(*freelist).next)
    return freelist;

  // Find the first node that is bigger than size
  metadata_t* curr = freelist;
  while ((*curr).size <= size) {
    curr = (*curr).next;
    if (!curr)
      return NULL;
    if ((*curr).size == size)
      return curr;
  }

  // Find the smallest node that is still bigger than size
  metadata_t* smallest = curr;
  while (curr) {
    if ((*smallest).size > (*curr).size && (*curr).size >= size)
      smallest = curr;
    curr = (*curr).next;
  }
  return smallest;
}

metadata_t* remove_from_free_list(metadata_t* curr) {
  // Makes sure curr is not null
  if (curr) {
    // If curr does not have a next or prev, it is the only node in the free list
    if (!(*curr).next && !(*curr).prev) {
      freelist = NULL;
    } else if (!(*curr).next && (*curr).prev) { // Curr does not have a next, but has a prev
      (*(*curr).prev).next = NULL;
      (*curr).prev = NULL;
    } else if ((*curr).next && !(*curr).prev) { // Curr has a next, but not a prev
      freelist = (*curr).next;
      (*(*curr).next).prev = NULL;
      (*curr).next = NULL;
    } else if ((*curr).next && (*curr).prev) { // Curr has both a next and a prev
      (*(*curr).next).prev = (*curr).prev;
      (*(*curr).prev).next = (*curr).next;
      (*curr).prev = NULL;
      (*curr).next = NULL;
    }
    // Return the node with next and prev updated
    return curr;
  }
  return NULL;
}