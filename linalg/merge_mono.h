#ifndef MERGE_MONO_H_
#define MERGE_MONO_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void addRowsAndUpdate(sparse_mat_t *mat, int i1, int i2, int len);
extern void removeColDefinitely(report_t *rep, sparse_mat_t *mat, int32_t j);
extern void removeCellAndUpdate(sparse_mat_t *mat, int i, int32_t j);
extern void destroyRow(sparse_mat_t *mat, int i);
extern int removeSingletons(report_t *rep, sparse_mat_t *mat);
extern int deleteEmptyColumns(sparse_mat_t *mat);
extern void removeRowDefinitely(report_t *rep, sparse_mat_t *mat, int32_t i);
extern int minColWeight(sparse_mat_t *mat);
extern void fillRowAddMatrix(int A[MERGE_LEVEL_MAX][MERGE_LEVEL_MAX], sparse_mat_t *mat, int m, int32_t *ind);
extern void MSTWithA(report_t *rep, sparse_mat_t *mat, int m, int32_t *ind, double *tMST, int A[MERGE_LEVEL_MAX][MERGE_LEVEL_MAX]);
extern int deleteHeavyColumns(report_t *rep, sparse_mat_t *mat);
extern int addFatherToSons(int history[MERGE_LEVEL_MAX][MERGE_LEVEL_MAX+1], sparse_mat_t *mat, int m, int *ind,	int A[MERGE_LEVEL_MAX][MERGE_LEVEL_MAX], int *father, int *height MAYBE_UNUSED, int hmax MAYBE_UNUSED, int sons[MERGE_LEVEL_MAX][MERGE_LEVEL_MAX+1]);

extern int cmp(const void *p, const void *q);
extern int number_of_superfluous_rows(sparse_mat_t *mat);
extern void merge(report_t *rep, sparse_mat_t *mat, int maxlevel, int verbose, int forbw);
extern void mergeOneByOne(report_t *rep, sparse_mat_t *mat, int maxlevel, int verbose, int forbw, double ratio, int coverNmax);
extern void doOneMerge(report_t *rep, sparse_mat_t *mat, int *njrem, double *totopt, double *totfill, double *totMST, double *totdel, int m, int maxdo, int useMST, int verbose);

extern void resume(report_t *rep, sparse_mat_t *mat, char *resumename);

#ifdef __cplusplus
}
#endif

#endif	/* MERGE_MONO_H_ */
