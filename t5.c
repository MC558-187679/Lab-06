/**
 * MC558 - Teste 05
 * Tiago de Paula - RA 187679
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __GNUC__
// Atributos do GCC
#define attribute(...) __attribute__((__VA_ARGS__))
// Marcador de branch improvável (usado para erros)
#define unlikely(x)    (__builtin_expect((x), 0))

#else
// Fora do GCC, as macros não fazem nada.
#define attribute(...)
#define unlikely(x)      (x)
#endif


/* Representação de uma aresta do grafo. */
typedef struct aresta {
    uint16_t A, B;  // computadores
    uint16_t W;     // custo da conexão
} aresta_t;

/* Representação de uma aresta do grafo. */
typedef struct grafo {
    unsigned N, M;  // dimensões do grafo
    aresta_t E[];   // arestas
} grafo_t;


/* * * * * * * * * *
 * ENTRADA E SAÍDA *
 * * * * * * * * * */

static inline attribute(malloc, hot, nothrow)
/**
 *  Construção de um grafo de 'N' vértices
 * e leitura dos suas 'M' arestas.
 *
 * Retorna NULL em caso de erro.
 */
grafo_t *ler_grafo(unsigned N, unsigned M) {
    if unlikely(N > UINT16_MAX) return NULL;
    // alocação do grafo
    size_t fixo = offsetof(grafo_t, E);
    grafo_t *grafo = malloc(fixo + M * sizeof(aresta_t));
    if unlikely(grafo == NULL) return NULL;
    // inicializa sem arestas
    grafo->N = N;
    grafo->M = 0;

    for (unsigned i = 0; i < M; i++) {
        unsigned A, B, W;
        // leitura dos dados
        int rv = scanf("%u %u %u", &A, &B, &W);
        // EOF não é considerado erro
        if unlikely(rv < 0) return grafo;
        // mas a aresta deve ser válida
        if unlikely(rv < 3 || A >= N || B >= N || W > UINT16_MAX) {
            free(grafo);
            return NULL;
        }
        // insere a nova aresta
        grafo->E[grafo->M++] = (aresta_t) {
            .A = A, .B = B, .W = W
        };
    }
    return grafo;
}

static inline
/**
 *  Cálculo do custo total para a rede de entrada,
 * respeitando os k clusters.
 *
 * Retorna UINT64_MAX em caso de erro;
 */
uint64_t custo_total(grafo_t *rede, unsigned K)
attribute(pure, nonnull, hot, nothrow);

int main(void) {
    unsigned N, M, K;
    // leitura dos parâmetros
    int rv = scanf("%u %u %u", &N, &M, &K);
    if unlikely(rv < 3 || K > N || K == 0) return EXIT_FAILURE;
    // leitura das arestas
    grafo_t *grafo = ler_grafo(N, M);
    if unlikely(grafo == NULL) return EXIT_FAILURE;
    // cálculo do custo
    uint64_t custo = custo_total(grafo, K);
    free(grafo);
    if unlikely(custo == UINT64_MAX) return EXIT_FAILURE;
    // exibição do resultado
    printf("%zu\n", custo);
    return EXIT_SUCCESS;
}


/* * * * * * * * *
 * CUSTO DA REDE *
 * * * * * * * * */

/**
 *  Representação de conjuntos
 * disjuntos por matriz booleana.
 */
typedef struct conjuntos {
    size_t tam;              // qtde de conjuntos
    uint16_t *restrict pos;  // conjunto atual do no 'i'
    bool *restrict elem;     // matriz marcando elementos
} conj_t;


static inline attribute(nonnull, hot, nothrow)
/**
 * Faz a união de dois conjuntos 'a != b'.
 */
void uniao(conj_t conj, unsigned a, unsigned b) {
    // sempre usa 'a' como o menor
    if (b < a) {
        unsigned x = b;
        b = a; a = x;
    }

    bool *ca = conj.elem + (a * conj.tam);
    bool *cb = conj.elem + (b * conj.tam);
    // para cada elemento de 'b'
    unsigned tam = conj.tam;
    for (unsigned i = 0; i < tam; i++) {
        if (cb[i]) {
            // insere em 'a'
            ca[i] = true;
            conj.pos[i] = a;
        }
    }
}

static inline attribute(nonnull, hot, nothrow)
/**
 *  Algoritmo de Kruskal modificado que encerra quando
 * a floresta é reduzida para 'max' árvores.
 */
uint64_t kruskal(const grafo_t *restrict rede, unsigned max, conj_t conj) {
    unsigned arvores = conj.tam;
    // checa se o limite já foi alcançado
    if unlikely(arvores < max) return UINT64_MAX;
    if unlikely(arvores == max) return 0;

    uint64_t custo = 0;
    unsigned M = rede->M;
    // assume as arestas já ordernadas
    for (unsigned e = 0; e < M; e++) {
        aresta_t a = rede->E[e];
        // acessa o conjnto de A e B
        unsigned ca = conj.pos[a.A];
        unsigned cb = conj.pos[a.B];
        // e junta se necessário
        if unlikely(ca != cb) {
            uniao(conj, ca, cb);
            custo += a.W;
            // encerra se alcançar o limite
            if unlikely(--arvores == max) {
                return custo;
            }
        }
    }
    return custo;
}

static attribute(nonnull, hot, nothrow)
/**
 * Ordenação das arestas por peso.
 */
void quicksort(aresta_t aresta[], unsigned hi) {
    // particiona o vetor
    aresta_t pivo = aresta[hi];
    unsigned i = 0;
    for (unsigned j = 0; j < hi; j++) {
        aresta_t cur = aresta[j];
        if (cur.W < pivo.W) {
            aresta[j] = aresta[i];
            aresta[i] = cur;
            i += 1;
        }
    }
    aresta[hi] = aresta[i];
    aresta[i] = pivo;
    // e ordena recursivamente
    if (i > 1) {
        quicksort(aresta, i - 1);
    }
    if (hi > (i + 1)) {
        quicksort(aresta + (i+1), hi - (i+1));
    }
}

static inline attribute(pure, nonnull, hot, nothrow)
/* Custo da rede */
uint64_t custo_total(grafo_t *rede, unsigned K) {
    unsigned N = rede->N;
    if unlikely(rede->M == 0) {
        return (K == N)? 0 : UINT64_MAX;
    }
    // alocação dos conjuntos
    uint16_t *pos = malloc(N * sizeof(uint16_t));
    if unlikely(pos == NULL) return UINT64_MAX;
    bool *elem = calloc(N * N, sizeof(bool));
    if unlikely(elem == NULL) {
        free(pos);
        return UINT64_MAX;
    }
    // prepara o conjunto inicial de cada nó
    conj_t conj = { .tam = N, .pos = pos, .elem = elem };
    for (unsigned i = 0; i < N; i++) {
        pos[i] = i;
        elem[i * (N + 1)] = true;
    }
    // ordenação das arestas
    quicksort(rede->E, rede->M - 1);
    // cálculo por Kruskal
    uint64_t custo = kruskal(rede, K, conj);
    free(pos);
    free(elem);
    return custo;
}
