/***************************************************************************/
/*                                                                         */
/*  ftcmanag.c                                                             */
/*                                                                         */
/*    FreeType Cache Manager (body).                                       */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2004, 2005 by                         */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_CACHE_H
#include FT_CACHE_INTERNAL_MANAGER_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_SIZES_H

#include "ftcerror.h"


#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cache

#define FTC_LRU_GET_MANAGER( lru )  ( (FTC_Manager)(lru)->user_data )


  static FT_Error
  ftc_scaler_lookup_size( FTC_Manager  manager,
                          FTC_Scaler   scaler,
                          FT_Size     *asize )
  {
    FT_Face   face;
    FT_Size   size = NULL;
    FT_Error  error;


    error = FTC_Manager_LookupFace( manager, scaler->face_id, &face );
    if ( error )
      goto Exit;

    error = FT_New_Size( face, &size );
    if ( error )
      goto Exit;

    FT_Activate_Size( size );

    if ( scaler->pixel )
      error = FT_Set_Pixel_Sizes( face, scaler->width, scaler->height );
    else
      error = FT_Set_Char_Size( face, scaler->width, scaler->height,
                                scaler->x_res, scaler->y_res );
    if ( error )
    {
      FT_Done_Size( size );
      size = NULL;
    }

  Exit:
    *asize = size;
    return error;
  }


  typedef struct  FTC_SizeNodeRec_
  {
    FTC_MruNodeRec  node;
    FT_Size         size;
    FTC_ScalerRec   scaler;

  } FTC_SizeNodeRec, *FTC_SizeNode;


  FT_CALLBACK_DEF( void )
  ftc_size_node_done( FTC_MruNode  ftcnode,
                      FT_Pointer   data )
  {
    FTC_SizeNode  node = (FTC_SizeNode)ftcnode;
    FT_Size       size = node->size;
    FT_UNUSED( data );


    if ( size )
      FT_Done_Size( size );
  }


  FT_CALLBACK_DEF( FT_Bool )
  ftc_size_node_compare( FTC_MruNode  ftcnode,
                         FT_Pointer   ftcscaler )
  {
    FTC_SizeNode  node    = (FTC_SizeNode)ftcnode;
    FTC_Scaler    scaler  = (FTC_Scaler)ftcscaler;
    FTC_Scaler    scaler0 = &node->scaler;


    if ( FTC_SCALER_COMPARE( scaler0, scaler ) )
    {
      FT_Activate_Size( node->size );
      return 1;
    }
    return 0;
  }


  FT_CALLBACK_DEF( FT_Error )
  ftc_size_node_init( FTC_MruNode  ftcnode,
                      FT_Pointer   ftcscaler,
                      FT_Pointer   ftcmanager )
  {
    FTC_SizeNode  node    = (FTC_SizeNode)ftcnode;
    FTC_Scaler    scaler  = (FTC_Scaler)ftcscaler;
    FTC_Manager   manager = (FTC_Manager)ftcmanager;


    node->scaler = scaler[0];

    return ftc_scaler_lookup_size( manager, scaler, &node->size );
  }


  FT_CALLBACK_DEF( FT_Error )
  ftc_size_node_reset( FTC_MruNode  ftcnode,
                       FT_Pointer   ftcscaler,
                       FT_Pointer   ftcmanager )
  {
    FTC_SizeNode  node    = (FTC_SizeNode)ftcnode;
    FTC_Scaler    scaler  = (FTC_Scaler)ftcscaler;
    FTC_Manager   manager = (FTC_Manager)ftcmanager;


    FT_Done_Size( node->size );

    node->scaler = scaler[0];

    return ftc_scaler_lookup_size( manager, scaler, &node->size );
  }


  FT_CALLBACK_TABLE_DEF
  const FTC_MruListClassRec  ftc_size_list_class =
  {
    sizeof ( FTC_SizeNodeRec ),
    ftc_size_node_compare,
    ftc_size_node_init,
    ftc_size_node_reset,
    ftc_size_node_done
  };


  /* helper function used by ftc_face_node_done */
  static FT_Bool
  ftc_size_node_compare_faceid( FTC_MruNode  ftcnode,
                                FT_Pointer   ftcface_id )
  {
    FTC_SizeNode  node    = (FTC_SizeNode)ftcnode;
    FTC_FaceID    face_id = (FTC_FaceID)ftcface_id;


    return FT_BOOL( node->scaler.face_id == face_id );
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_LookupSize( FTC_Manager  manager,
                          FTC_Scaler   scaler,
                          FT_Size     *asize )
  {
    FT_Error      error;
    FTC_SizeNode  node;


    if ( asize == NULL )
      return FTC_Err_Bad_Argument;

    *asize = NULL;

    if ( !manager )
      return FTC_Err_Invalid_Cache_Handle;

#ifdef FTC_INLINE

    FTC_MRULIST_LOOKUP_CMP( &manager->sizes, scaler, ftc_size_node_compare,
                            node, error );

#else
    error = FTC_MruList_Lookup( &manager->sizes, scaler, (FTC_MruNode*)&node );
#endif

    if ( !error )
      *asize = node->size;

    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    FACE MRU IMPLEMENTATION                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  typedef struct  FTC_FaceNodeRec_
  {
    FTC_MruNodeRec  node;
    FTC_FaceID      face_id;
    FT_Face         face;

  } FTC_FaceNodeRec, *FTC_FaceNode;


  FT_CALLBACK_DEF( FT_Error )
  ftc_face_node_init( FTC_MruNode  ftcnode,
                      FT_Pointer   ftcface_id,
                      FT_Pointer   ftcmanager )
  {
    FTC_FaceNode  node    = (FTC_FaceNode)ftcnode;
    FTC_FaceID    face_id = (FTC_FaceID)ftcface_id;
    FTC_Manager   manager = (FTC_Manager)ftcmanager;
    FT_Error      error;


    node->face_id = face_id;

    error = manager->request_face( face_id,
                                   manager->library,
                                   manager->request_data,
                                   &node->face );
    if ( !error )
    {
      /* destroy initial size object; it will be re-created later */
      if ( node->face->size )
        FT_Done_Size( node->face->size );
    }

    return error;
  }


  FT_CALLBACK_DEF( void )
  ftc_face_node_done( FTC_MruNode  ftcnode,
                      FT_Pointer   ftcmanager )
  {
    FTC_FaceNode  node    = (FTC_FaceNode)ftcnode;
    FTC_Manager   manager = (FTC_Manager)ftcmanager;


    /* we must begin by removing all scalers for the target face */
    /* from the manager's list                                   */
    FTC_MruList_RemoveSelection( &manager->sizes,
                                 ftc_size_node_compare_faceid,
                                 node->face_id );

    /* all right, we can discard the face now */
    FT_Done_Face( node->face );
    node->face    = NULL;
    node->face_id = NULL;
  }


  FT_CALLBACK_DEF( FT_Bool )
  ftc_face_node_compare( FTC_MruNode  ftcnode,
                         FT_Pointer   ftcface_id )
  {
    FTC_FaceNode  node    = (FTC_FaceNode)ftcnode;
    FTC_FaceID    face_id = (FTC_FaceID)ftcface_id;


    return FT_BOOL( node->face_id == face_id );
  }


  FT_CALLBACK_TABLE_DEF
  const FTC_MruListClassRec  ftc_face_list_class =
  {
    sizeof ( FTC_FaceNodeRec),

    ftc_face_node_compare,
    ftc_face_node_init,
    0,                          /* FTC_MruNode_ResetFunc */
    ftc_face_node_done
  };


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_LookupFace( FTC_Manager  manager,
                          FTC_FaceID   face_id,
                          FT_Face     *aface )
  {
    FT_Error      error;
    FTC_FaceNode  node;


    if ( aface == NULL )
      return FTC_Err_Bad_Argument;

    *aface = NULL;

    if ( !manager )
      return FTC_Err_Invalid_Cache_Handle;

    /* we break encapsulation for the sake of speed */
#ifdef FTC_INLINE

    FTC_MRULIST_LOOKUP_CMP( &manager->faces, face_id, ftc_face_node_compare,
                            node, error );

#else
    error = FTC_MruList_Lookup( &manager->faces, face_id, (FTC_MruNode*)&node );
#endif

    if ( !error )
      *aface = node->face;

    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    CACHE MANAGER ROUTINES                     *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_New( FT_Library          library,
                   FT_UInt             max_faces,
                   FT_UInt             max_sizes,
                   FT_ULong            max_bytes,
                   FTC_Face_Requester  requester,
                   FT_Pointer          req_data,
                   FTC_Manager        *amanager )
  {
    FT_Error     error;
    FT_Memory    memory;
    FTC_Manager  manager = 0;


    if ( !library )
      return FTC_Err_Invalid_Library_Handle;

    memory = library->memory;

    if ( FT_NEW( manager ) )
      goto Exit;

    if ( max_faces == 0 )
      max_faces = FTC_MAX_FACES_DEFAULT;

    if ( max_sizes == 0 )
      max_sizes = FTC_MAX_SIZES_DEFAULT;

    if ( max_bytes == 0 )
      max_bytes = FTC_MAX_BYTES_DEFAULT;

    manager->library      = library;
    manager->memory       = memory;
    manager->max_weight   = max_bytes;

    manager->request_face = requester;
    manager->request_data = req_data;

    FTC_MruList_Init( &manager->faces,
                      &ftc_face_list_class,
                      max_faces,
                      manager,
                      memory );

    FTC_MruList_Init( &manager->sizes,
                      &ftc_size_list_class,
                      max_sizes,
                      manager,
                      memory );

    *amanager = manager;

  Exit:
    return error;
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( void )
  FTC_Manager_Done( FTC_Manager  manager )
  {
    FT_Memory  memory;
    FT_UInt    idx;


    if ( !manager || !manager->library )
      return;

    memory = manager->memory;

    /* now discard all caches */
    for (idx = manager->num_caches; idx-- > 0; )
    {
      FTC_Cache  cache = manager->caches[idx];


      if ( cache )
      {
        cache->clazz.cache_done( cache );
        FT_FREE( cache );
        manager->caches[idx] = NULL;
      }
    }
    manager->num_caches = 0;

    /* discard faces and sizes */
    FTC_MruList_Done( &manager->sizes );
    FTC_MruList_Done( &manager->faces );

    manager->library = NULL;
    manager->memory  = NULL;

    FT_FREE( manager );
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( void )
  FTC_Manager_Reset( FTC_Manager  manager )
  {
    if ( manager )
    {
      FTC_MruList_Reset( &manager->sizes );
      FTC_MruList_Reset( &manager->faces );
    }
    /* XXX: FIXME: flush the caches? */
  }


#ifdef FT_DEBUG_ERROR

  FT_EXPORT_DEF( void )
  FTC_Manager_Check( FTC_Manager  manager )
  {
    FTC_Node  node, first;


    first = manager->nodes_list;

    /* check node weights */
    if ( first )
    {
      FT_ULong  weight = 0;


      node = first;

      do
      {
        FTC_Cache  cache = manager->caches[node->cache_index];


        if ( (FT_UInt)node->cache_index >= manager->num_caches )
          FT_ERROR(( "FTC_Manager_Check: invalid node (cache index = %ld\n",
                     node->cache_index ));
        else
          weight += cache->clazz.node_weight( node, cache );

        node = FTC_NODE__NEXT( node );

      } while ( node != first );

      if ( weight != manager->cur_weight )
        FT_ERROR(( "FTC_Manager_Check: invalid weight %ld instead of %ld\n",
                   manager->cur_weight, weight ));
    }

    /* check circular list */
    if ( first )
    {
      FT_UFast  count = 0;


      node = first;
      do
      {
        count++;
        node = FTC_NODE__NEXT( node );

      } while ( node != first );

      if ( count != manager->num_nodes )
        FT_ERROR((
          "FTC_Manager_Check: invalid cache node count %d instead of %d\n",
          manager->num_nodes, count ));
    }
  }

#endif /* FT_DEBUG_ERROR */


  /* `Compress' the manager's data, i.e., get rid of old cache nodes */
  /* that are not referenced anymore in order to limit the total     */
  /* memory used by the cache.                                       */

  /* documentation is in ftcmanag.h */

  FT_EXPORT_DEF( void )
  FTC_Manager_Compress( FTC_Manager  manager )
  {
    FTC_Node   node, first;


    if ( !manager )
      return;

    first = manager->nodes_list;

#ifdef FT_DEBUG_ERROR
    FTC_Manager_Check( manager );

    FT_ERROR(( "compressing, weight = %ld, max = %ld, nodes = %d\n",
               manager->cur_weight, manager->max_weight,
               manager->num_nodes ));
#endif

    if ( manager->cur_weight < manager->max_weight || first == NULL )
      return;

    /* go to last node -- it's a circular list */
    node = FTC_NODE__PREV( first );
    do
    {
      FTC_Node  prev;


      prev = ( node == first ) ? NULL : FTC_NODE__PREV( node );

      if ( node->ref_count <= 0 )
        ftc_node_destroy( node, manager );

      node = prev;

    } while ( node && manager->cur_weight > manager->max_weight );
  }


  /* documentation is in ftcmanag.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_RegisterCache( FTC_Manager      manager,
                             FTC_CacheClass   clazz,
                             FTC_Cache       *acache )
  {
    FT_Error   error = FTC_Err_Invalid_Argument;
    FTC_Cache  cache = NULL;


    if ( manager && clazz && acache )
    {
      FT_Memory  memory = manager->memory;


      if ( manager->num_caches >= FTC_MAX_CACHES )
      {
        error = FTC_Err_Too_Many_Caches;
        FT_ERROR(( "%s: too many registered caches\n",
                   "FTC_Manager_Register_Cache" ));
        goto Exit;
      }

      if ( !FT_ALLOC( cache, clazz->cache_size ) )
      {
        cache->manager   = manager;
        cache->memory    = memory;
        cache->clazz     = clazz[0];
        cache->org_class = clazz;

        /* THIS IS VERY IMPORTANT!  IT WILL WRETCH THE MANAGER */
        /* IF IT IS NOT SET CORRECTLY                          */
        cache->index = manager->num_caches;

        error = clazz->cache_init( cache );
        if ( error )
        {
          clazz->cache_done( cache );
          FT_FREE( cache );
          goto Exit;
        }

        manager->caches[manager->num_caches++] = cache;
      }
    }

  Exit:
    *acache = cache;
    return error;
  }


  FT_EXPORT_DEF( FT_UInt )
  FTC_Manager_FlushN( FTC_Manager  manager,
                      FT_UInt      count )
  {
    FTC_Node  first = manager->nodes_list;
    FTC_Node  node;
    FT_UInt   result;


    /* try to remove `count' nodes from the list */
    if ( first == NULL )  /* empty list! */
      return 0;

    /* go to last node - it's a circular list */
    node = FTC_NODE__PREV(first);
    for ( result = 0; result < count; )
    {
      FTC_Node  prev = FTC_NODE__PREV( node );


      /* don't touch locked nodes */
      if ( node->ref_count <= 0 )
      {
        ftc_node_destroy( node, manager );
        result++;
      }

      if ( node == first )
        break;

      node = prev;
    }
    return  result;
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( void )
  FTC_Manager_RemoveFaceID( FTC_Manager  manager,
                            FTC_FaceID   face_id )
  {
    FT_UInt  nn;

    /* this will remove all FTC_SizeNode that correspond to
     * the face_id as well
     */
    FTC_MruList_RemoveSelection( &manager->faces, NULL, face_id );

    for ( nn = 0; nn < manager->num_caches; nn++ )
      FTC_Cache_RemoveFaceID( manager->caches[nn], face_id );
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( void )
  FTC_Node_Unref( FTC_Node     node,
                  FTC_Manager  manager )
  {
    if ( node && (FT_UInt)node->cache_index < manager->num_caches )
      node->ref_count--;
  }


/* END */
