/**
 * \file parmmg.h
 * \brief internal functions headers for parmmg
 * \author Cécile Dobrzynski (Bx INP/Inria)
 * \author Algiane Froehly (Inria)
 * \version 5
 * \copyright GNU Lesser General Public License.
 */

#ifndef _PARMMG_H
#define _PARMMG_H

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <mpi_pmmg.h>

#include "libparmmg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \def PMMG_NUL
 *
 * Null value
 *
 */
#define PMMG_NUL     0

/**
 * \def PMMG_NITER
 *
 * Default number of iterations
 *
 */
#define PMMG_NITER   1

/**
 * \def PMMG_IMPRIM
 *
 * Default verbosity
 *
 */
#define PMMG_IMPRIM   1


/**
 * \param parmesh pointer toward a parmesh structure
 * \param val     exit value
 *
 * Controlled parmmg termination:
 *   Deallocate parmesh struct and its allocated members
 *   If this is an unsuccessful exit call abort to cancel any remaining processes
 *   Call MPI_Finalize / exit
 */

#define PMMG_RETURN_AND_FREE(parmesh,val) do                            \
  {                                                                     \
    MPI_Comm comm = parmesh->comm;                                      \
                                                                        \
    if ( !PMMG_Free_all( PMMG_ARG_start,                                \
                         PMMG_ARG_ppParMesh,&parmesh,                   \
                         PMMG_ARG_end) ) {                              \
      fprintf(stderr,"  ## Warning: unable to clean the parmmg memory.\n" \
              " Possible memory leak.\n");                              \
    }                                                                   \
                                                                        \
    MPI_Finalize();                                                     \
    return(val);                                                        \
                                                                        \
  } while(0)

/**
 * Clean the mesh, the metric and the solutions and return \a val.
 */
#define PMMG_CLEAN_AND_RETURN(parmesh,val)do                            \
  {                                                                     \
                                                                        \
    for ( int kgrp=0; kgrp<parmesh->ngrp; ++kgrp ) {                    \
      if ( parmesh->listgrp[kgrp].mesh ) {                              \
        parmesh->listgrp[kgrp].mesh->npi = parmesh->listgrp[kgrp].mesh->np; \
        parmesh->listgrp[kgrp].mesh->nti = parmesh->listgrp[kgrp].mesh->nt; \
        parmesh->listgrp[kgrp].mesh->nai = parmesh->listgrp[kgrp].mesh->na; \
        parmesh->listgrp[kgrp].mesh->nei = parmesh->listgrp[kgrp].mesh->ne; \
      }                                                                 \
                                                                        \
      if ( parmesh->listgrp[kgrp].met )                                 \
        parmesh->listgrp[kgrp].met->npi  = parmesh->listgrp[kgrp].met->np; \
                                                                        \
      if ( parmesh->listgrp[kgrp].mesh ) {                              \
        for ( int ksol=0; ksol<parmesh->listgrp[kgrp].mesh->nsols; ++ksol ) { \
          parmesh->listgrp[kgrp].sol[ksol].npi  = parmesh->listgrp[kgrp].sol[ksol].np; \
        }                                                               \
      }                                                                 \
    }                                                                   \
                                                                        \
    return val;                                                         \
                                                                        \
  }while(0)


#define ERROR_AT(msg1,msg2)                                          \
  fprintf( stderr, msg1 msg2 " function: %s, file: %s, line: %d \n", \
           __func__, __FILE__, __LINE__ )

#define MEM_CHK_AVAIL(mesh,bytes,msg) do {                            \
  if ( (mesh)->memCur + (bytes) > (mesh)->memMax ) {                  \
    ERROR_AT(msg," Exceeded max memory allowed: ");      \
    stat = PMMG_FAILURE;                                              \
  } else if ( (mesh)->memCur + (bytes) < 0  ) {                       \
    ERROR_AT(msg," Tried to free more mem than allocated: " );        \
    stat = PMMG_SUCCESS;                                              \
  }                                                                   \
  else {                                                              \
    stat = PMMG_SUCCESS;                                              \
  } } while(0)

#define PMMG_DEL_MEM(mesh,ptr,size,type,msg) do {           \
    int stat = PMMG_SUCCESS;                                \
    if ( (size) != 0 && ptr ) {                             \
      long long size_to_free = -(size)*sizeof(type);        \
      MEM_CHK_AVAIL(mesh,size_to_free,msg);                 \
      if ( stat == PMMG_SUCCESS )                           \
        (mesh)->memCur += size_to_free;                     \
    }                                                       \
    if ( ptr ) {                                            \
      free( ptr );                                          \
      (ptr) = NULL;                                         \
    }                                                       \
  } while(0)

#define PMMG_MALLOC(mesh,ptr,size,type,msg,on_failure) do { \
  int stat = PMMG_SUCCESS;                                  \
  (ptr) = NULL;                                             \
  if ( (size) != 0 ) {                                      \
    long long size_to_allocate = (size)*sizeof(type);       \
    MEM_CHK_AVAIL(mesh,size_to_allocate,msg );              \
    if ( stat == PMMG_SUCCESS ) {                           \
      (ptr) = malloc( size_to_allocate );                   \
      if ( (ptr) == NULL ) {                                \
        ERROR_AT( msg, " malloc failed: " );                \
        on_failure;                                         \
      } else {                                              \
        (mesh)->memCur += size_to_allocate;                 \
        stat = PMMG_SUCCESS;                                \
      }                                                     \
    } else {                                                \
      on_failure;                                           \
    }                                                       \
  } } while(0)

#define PMMG_CALLOC(mesh,ptr,size,type,msg,on_failure) do { \
  int stat = PMMG_SUCCESS;                                  \
  (ptr) = NULL;                                             \
  if ( (size) != 0 ) {                                      \
    long long size_to_allocate = (size)*sizeof(type);       \
    MEM_CHK_AVAIL(mesh,size_to_allocate,msg);               \
    if ( stat == PMMG_SUCCESS ) {                           \
      (ptr) = calloc( (size), sizeof(type) );               \
      if ( (ptr) == NULL ) {                                \
        ERROR_AT(msg," calloc failed: ");                   \
        on_failure;                                         \
      } else {                                              \
        (mesh)->memCur += size_to_allocate;                 \
      }                                                     \
    } else {                                                \
      on_failure;                                           \
    }                                                       \
  } } while(0)

#define PMMG_REALLOC(mesh,ptr,newsize,oldsize,type,msg,on_failure) do { \
    int   stat = PMMG_SUCCESS;                                          \
    type* tmp;                                                          \
                                                                        \
    if ( (ptr) == NULL ) {                                              \
      assert(((oldsize)==0) && "NULL pointer pointing to non 0 sized memory?");\
      PMMG_MALLOC(mesh,ptr,(newsize),type,msg,on_failure);              \
    } else if ((newsize)==0) {                                          \
      PMMG_DEL_MEM(mesh,ptr,(oldsize),type,msg);                        \
    } else if ((newsize) < (oldsize)) {                                 \
      long long size_to_allocate = (newsize)*sizeof(type);              \
      tmp = (type *)realloc((ptr),(long long)(newsize)*sizeof(type));   \
      if ( tmp == NULL ) {                                              \
        ERROR_AT(msg," Realloc failed: ");                              \
        PMMG_DEL_MEM(mesh,ptr,(oldsize),type,msg);                      \
        on_failure;                                                     \
      } else {                                                          \
        (ptr) = tmp;                                                    \
        (mesh)->memCur -= ((long long)((oldsize)*sizeof(type))-size_to_allocate);\
      }                                                                 \
    } else if ((newsize) > (oldsize)) {                                 \
long long size_to_allocate = ((long long)(newsize)-(long long)(oldsize))*sizeof(type);\
      MEM_CHK_AVAIL(mesh,size_to_allocate,msg);                         \
      if ( stat == PMMG_SUCCESS ) {                                     \
        tmp = (type *)realloc((ptr),(long long)(newsize)*sizeof(type)); \
        if ( tmp == NULL ) {                                            \
          ERROR_AT(msg, " Realloc failed: " );                          \
          PMMG_DEL_MEM(mesh,ptr,(oldsize),type,msg);                    \
          on_failure;                                                   \
        } else {                                                        \
          (ptr) = tmp;                                                  \
          (mesh)->memCur += ( size_to_allocate );                       \
        }                                                               \
      }                                                                 \
    }                                                                   \
  } while(0)

#define PMMG_RECALLOC(mesh,ptr,newsize,oldsize,type,msg,on_failure) do { \
  int my_stat = PMMG_SUCCESS;                                            \
  PMMG_REALLOC(mesh,ptr,newsize,oldsize,type,msg,on_failure;my_stat=PMMG_FAILURE);\
  if ( (my_stat == PMMG_SUCCESS ) && ((newsize) > (oldsize)) )           \
    memset( (ptr) + oldsize, 0, ((size_t)((newsize)-(oldsize)))*sizeof(type));     \
  } while(0)


/* Input */
int PMMG_check_inputData ( PMMG_pParMesh parmesh );
int PMMG_parsar( int argc, char *argv[], PMMG_pParMesh parmesh );

/* Internal library */
int PMMG_parmmglib1 ( PMMG_pParMesh parmesh );

/* Mesh distrib */
int PMMG_bdryUpdate( MMG5_pMesh mesh );
int PMMG_bcast_mesh ( PMMG_pParMesh parmesh );
int PMMG_grpSplit_setMeshSize( MMG5_pMesh,int,int,int,int,int );
int PMMG_split_grps( PMMG_pParMesh,int,int );

/* Load Balancing */
int PMMG_distribute_grps( PMMG_pParMesh parmesh );
int PMMG_loadBalancing( PMMG_pParMesh parmesh );
int PMMG_split_n2mGrps( PMMG_pParMesh,int,int );

/* Communicators building */
int PMMG_build_nodeCommFromFaces( PMMG_pParMesh parmesh );
int PMMG_build_simpleExtNodeComm( PMMG_pParMesh parmesh );
int PMMG_build_intNodeComm( PMMG_pParMesh parmesh );
int PMMG_build_completeExtNodeComm( PMMG_pParMesh parmesh );

/* Communicators checks */
int PMMG_check_intFaceComm( PMMG_pParMesh parmesh );
int PMMG_check_extFaceComm( PMMG_pParMesh parmesh );
int PMMG_check_intNodeComm( PMMG_pParMesh parmesh );
int PMMG_check_extNodeComm( PMMG_pParMesh parmesh );

/* Mesh merge */
int PMMG_mergeGrpJinI_interfacePoints_addGrpJ( PMMG_pParMesh,PMMG_pGrp,PMMG_pGrp);
int PMMG_mergeGrps_interfacePoints( PMMG_pParMesh parmesh );
int PMMG_mergeGrpJinI_internalPoints( PMMG_pGrp,PMMG_pGrp grpJ );
int PMMG_mergeGrpJinI_interfaceTetra( PMMG_pParMesh,PMMG_pGrp,PMMG_pGrp );
int PMMG_mergeGrpJinI_internalTetra( PMMG_pGrp,PMMG_pGrp );
int PMMG_merge_grps ( PMMG_pParMesh parmesh );

/* Packing */
int PMMG_update_node2intPackedTetra( PMMG_pGrp grp );
int PMMG_mark_packedTetra(MMG5_pMesh mesh,int *ne);
int PMMG_update_node2intPackedVertices( PMMG_pGrp grp );
int PMMG_packTetra ( PMMG_pParMesh parmesh, int igrp );

/* Memory */
int  PMMG_link_mesh( MMG5_pMesh mesh );
void PMMG_listgrp_free( PMMG_pParMesh parmesh, PMMG_pGrp *listgrp, int ngrp );
void PMMG_grp_free( PMMG_pParMesh parmesh, PMMG_pGrp grp );
int  PMMG_parmesh_SetMemMax( PMMG_pParMesh parmesh, int percent);
int  PMMG_parmesh_updateMemMax( PMMG_pParMesh parmesh,int percent,int fitMesh);
void PMMG_parmesh_SetMemGloMax( PMMG_pParMesh parmesh, long long int memReq );
void PMMG_parmesh_Free_Comm( PMMG_pParMesh parmesh );
void PMMG_parmesh_Free_Listgrp( PMMG_pParMesh parmesh );
int  PMMG_clean_emptyMesh( PMMG_pParMesh parmesh, PMMG_pGrp listgrp, int ngrp );

/* Scaling */
int PMMG_scaleMesh(MMG5_pMesh mesh,MMG5_pSol met);

/* Quality */
int PMMG_outqua( PMMG_pParMesh parmesh );

/* Variadic_pmmg.c */
int PMMG_Init_parMesh_var_internal(va_list argptr,int callFromC);
int PMMG_Free_all_var(va_list argptr);

const char* PMMG_Get_pmmgArgName(int typArg);


#ifdef __cplusplus
}
#endif

#endif
