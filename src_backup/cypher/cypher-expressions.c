/*
 * Cypher Expression Evaluation Implementation
 * Core expression system for openCypher compliance
 */

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <sqlite3.h>
#include "cypher-expressions.h"

/* Global function registry */
static CypherBuiltinFunction *g_functions = NULL;
static int g_nFunctions = 0;

/* Expression creation */
int cypherExpressionCreate(CypherExpression **ppExpr, CypherExpressionType type) {
    CypherExpression *pExpr;
    
    if (!ppExpr) return SQLITE_MISUSE;
    
    pExpr = sqlite3_malloc(sizeof(CypherExpression));
    if (!pExpr) return SQLITE_NOMEM;
    
    memset(pExpr, 0, sizeof(CypherExpression));
    pExpr->type = type;
    
    *ppExpr = pExpr;
    return SQLITE_OK;
}

int cypherExpressionDestroy(CypherExpression *pExpr) {
    int i;
    
    if (!pExpr) return SQLITE_OK;
    
    switch (pExpr->type) {
        case CYPHER_EXPR_LITERAL:
            cypherValueDestroy(&pExpr->u.literal);
            break;
            
        case CYPHER_EXPR_VARIABLE:
            sqlite3_free(pExpr->u.variable.zName);
            break;
            
        case CYPHER_EXPR_PROPERTY:
            cypherExpressionDestroy(pExpr->u.property.pObject);
            sqlite3_free(pExpr->u.property.zProperty);
            break;
            
        case CYPHER_EXPR_ARITHMETIC:
        case CYPHER_EXPR_COMPARISON:
        case CYPHER_EXPR_LOGICAL:
        case CYPHER_EXPR_STRING:
            cypherExpressionDestroy(pExpr->u.binary.pLeft);
            cypherExpressionDestroy(pExpr->u.binary.pRight);
            break;
            
        case CYPHER_EXPR_FUNCTION:
            sqlite3_free(pExpr->u.function.zName);
            for (i = 0; i < pExpr->u.function.nArgs; i++) {
                cypherExpressionDestroy(pExpr->u.function.apArgs[i]);
            }
            sqlite3_free(pExpr->u.function.apArgs);
            break;
            
        case CYPHER_EXPR_LIST:
            for (i = 0; i < pExpr->u.list.nElements; i++) {
                cypherExpressionDestroy(pExpr->u.list.apElements[i]);
            }
            sqlite3_free(pExpr->u.list.apElements);
            break;
            
        case CYPHER_EXPR_MAP:
            for (i = 0; i < pExpr->u.map.nPairs; i++) {
                sqlite3_free(pExpr->u.map.azKeys[i]);
                cypherExpressionDestroy(pExpr->u.map.apValues[i]);
            }
            sqlite3_free(pExpr->u.map.azKeys);
            sqlite3_free(pExpr->u.map.apValues);
            break;
            
        default:
            break;
    }
    
    sqlite3_free(pExpr);
    return SQLITE_OK;
}

/* Expression evaluation */
int cypherExpressionEvaluate(const CypherExpression *pExpr, 
                            ExecutionContext *pContext, 
                            CypherValue *pResult) {
    CypherValue left, right;
    int rc = SQLITE_OK;
    int i;
    
    if (!pExpr || !pResult) return SQLITE_MISUSE;
    
    /* Initialize result */
    cypherValueInit(pResult);
    
    switch (pExpr->type) {
        case CYPHER_EXPR_LITERAL:
            {
                CypherValue *pCopy = cypherValueCopy(&pExpr->u.literal);
                if( pCopy ) {
                    *pResult = *pCopy;
                    sqlite3_free(pCopy);
                    return SQLITE_OK;
                } else {
                    return SQLITE_NOMEM;
                }
            }
            
        case CYPHER_EXPR_VARIABLE:
            /* Look up variable in execution context */
            if (pContext) {
                /* TODO: Implement variable lookup */
                cypherValueSetNull(pResult);
            } else {
                cypherValueSetNull(pResult);
            }
            return SQLITE_OK;
            
        case CYPHER_EXPR_ARITHMETIC:
            cypherValueInit(&left);
            cypherValueInit(&right);
            
            rc = cypherExpressionEvaluate(pExpr->u.binary.pLeft, pContext, &left);
            if (rc != SQLITE_OK) goto arithmetic_cleanup;
            
            rc = cypherExpressionEvaluate(pExpr->u.binary.pRight, pContext, &right);
            if (rc != SQLITE_OK) goto arithmetic_cleanup;
            
            rc = cypherEvaluateArithmetic(&left, &right, 
                                        (CypherArithmeticOp)pExpr->u.binary.op, 
                                        pResult);
                                        
        arithmetic_cleanup:
            cypherValueDestroy(&left);
            cypherValueDestroy(&right);
            return rc;
            
        case CYPHER_EXPR_COMPARISON:
            cypherValueInit(&left);
            cypherValueInit(&right);
            
            rc = cypherExpressionEvaluate(pExpr->u.binary.pLeft, pContext, &left);
            if (rc != SQLITE_OK) goto comparison_cleanup;
            
            rc = cypherExpressionEvaluate(pExpr->u.binary.pRight, pContext, &right);
            if (rc != SQLITE_OK) goto comparison_cleanup;
            
            rc = cypherEvaluateComparison(&left, &right, 
                                        (CypherComparisonOp)pExpr->u.binary.op, 
                                        pResult);
                                        
        comparison_cleanup:
            cypherValueDestroy(&left);
            cypherValueDestroy(&right);
            return rc;
            
        case CYPHER_EXPR_FUNCTION:
            return cypherEvaluateFunction(pExpr->u.function.zName,
                                        pExpr->u.function.apArgs,
                                        pExpr->u.function.nArgs,
                                        pContext,
                                        pResult);
            
        default:
            cypherValueSetNull(pResult);
            return SQLITE_OK;
    }
}

/* Arithmetic evaluation */
int cypherEvaluateArithmetic(const CypherValue *pLeft, const CypherValue *pRight,
                           CypherArithmeticOp op, CypherValue *pResult) {
    double leftVal, rightVal, result;
    
    if (!pLeft || !pRight || !pResult) return SQLITE_MISUSE;
    
    /* Handle NULL values */
    if (pLeft->type == CYPHER_VALUE_NULL || pRight->type == CYPHER_VALUE_NULL) {
        cypherValueSetNull(pResult);
        return SQLITE_OK;
    }
    
    /* Convert to numeric values */
    if (pLeft->type == CYPHER_VALUE_INTEGER) {
        leftVal = (double)pLeft->u.iInteger;
    } else if (pLeft->type == CYPHER_VALUE_FLOAT) {
        leftVal = pLeft->u.rFloat;
    } else {
        return SQLITE_MISMATCH; /* Type error */
    }
    
    if (pRight->type == CYPHER_VALUE_INTEGER) {
        rightVal = (double)pRight->u.iInteger;
    } else if (pRight->type == CYPHER_VALUE_FLOAT) {
        rightVal = pRight->u.rFloat;
    } else {
        return SQLITE_MISMATCH; /* Type error */
    }
    
    /* Perform operation */
    switch (op) {
        case CYPHER_OP_ADD:
            result = leftVal + rightVal;
            break;
        case CYPHER_OP_SUBTRACT:
            result = leftVal - rightVal;
            break;
        case CYPHER_OP_MULTIPLY:
            result = leftVal * rightVal;
            break;
        case CYPHER_OP_DIVIDE:
            if (rightVal == 0.0) {
                cypherValueSetNull(pResult);
                return SQLITE_OK;
            }
            result = leftVal / rightVal;
            break;
        case CYPHER_OP_MODULO:
            if (rightVal == 0.0) {
                cypherValueSetNull(pResult);
                return SQLITE_OK;
            }
            result = fmod(leftVal, rightVal);
            break;
        case CYPHER_OP_POWER:
            result = pow(leftVal, rightVal);
            break;
        default:
            return SQLITE_ERROR;
    }
    
    /* Return result as appropriate type */
    if (pLeft->type == CYPHER_VALUE_INTEGER && pRight->type == CYPHER_VALUE_INTEGER &&
        op != CYPHER_OP_DIVIDE && op != CYPHER_OP_POWER) {
        cypherValueSetInteger(pResult, (sqlite3_int64)result);
    } else {
        cypherValueSetFloat(pResult, result);
    }
    
    return SQLITE_OK;
}

/* Comparison evaluation */
int cypherEvaluateComparison(const CypherValue *pLeft, const CypherValue *pRight,
                           CypherComparisonOp op, CypherValue *pResult) {
    int cmp;
    int result;
    
    if (!pLeft || !pRight || !pResult) return SQLITE_MISUSE;
    
    /* Handle NULL comparisons */
    if (op == CYPHER_CMP_IS_NULL) {
        cypherValueSetBoolean(pResult, pLeft->type == CYPHER_VALUE_NULL);
        return SQLITE_OK;
    }
    if (op == CYPHER_CMP_IS_NOT_NULL) {
        cypherValueSetBoolean(pResult, pLeft->type != CYPHER_VALUE_NULL);
        return SQLITE_OK;
    }
    
    if (pLeft->type == CYPHER_VALUE_NULL || pRight->type == CYPHER_VALUE_NULL) {
        cypherValueSetNull(pResult);
        return SQLITE_OK;
    }
    
    /* Compare values */
    cmp = cypherValueCompare(pLeft, pRight);
    
    switch (op) {
        case CYPHER_CMP_EQUAL:
            result = (cmp == 0);
            break;
        case CYPHER_CMP_NOT_EQUAL:
            result = (cmp != 0);
            break;
        case CYPHER_CMP_LESS:
            result = (cmp < 0);
            break;
        case CYPHER_CMP_LESS_EQUAL:
            result = (cmp <= 0);
            break;
        case CYPHER_CMP_GREATER:
            result = (cmp > 0);
            break;
        case CYPHER_CMP_GREATER_EQUAL:
            result = (cmp >= 0);
            break;
        default:
            return SQLITE_ERROR;
    }
    
    cypherValueSetBoolean(pResult, result);
    return SQLITE_OK;
}

/* Function evaluation */
int cypherEvaluateFunction(const char *zName, CypherExpression **apArgs, int nArgs,
                         ExecutionContext *pContext, CypherValue *pResult) {
    const CypherBuiltinFunction *pFunc;
    CypherValue *aValues = NULL;
    int rc = SQLITE_OK;
    int i;
    
    if (!zName || !pResult) return SQLITE_MISUSE;
    
    /* Look up function */
    pFunc = cypherGetBuiltinFunction(zName);
    if (!pFunc) {
        return SQLITE_ERROR; /* Unknown function */
    }
    
    /* Check argument count */
    if (nArgs < pFunc->nMinArgs || (pFunc->nMaxArgs >= 0 && nArgs > pFunc->nMaxArgs)) {
        return SQLITE_ERROR; /* Wrong argument count */
    }
    
    /* Evaluate arguments */
    if (nArgs > 0) {
        aValues = sqlite3_malloc(sizeof(CypherValue) * nArgs);
        if (!aValues) return SQLITE_NOMEM;
        
        for (i = 0; i < nArgs; i++) {
            cypherValueInit(&aValues[i]);
            rc = cypherExpressionEvaluate(apArgs[i], pContext, &aValues[i]);
            if (rc != SQLITE_OK) {
                /* Cleanup on error */
                for (int j = 0; j < i; j++) {
                    cypherValueDestroy(&aValues[j]);
                }
                sqlite3_free(aValues);
                return rc;
            }
        }
    }
    
    /* Call function */
    rc = pFunc->xFunction(aValues, nArgs, pResult);
    
    /* Cleanup */
    if (aValues) {
        for (i = 0; i < nArgs; i++) {
            cypherValueDestroy(&aValues[i]);
        }
        sqlite3_free(aValues);
    }
    
    return rc;
}

/* Literal expression creation */
int cypherExpressionCreateLiteral(CypherExpression **ppExpr, const CypherValue *pValue) {
    CypherExpression *pExpr;
    int rc;
    
    if (!ppExpr || !pValue) return SQLITE_MISUSE;
    
    rc = cypherExpressionCreate(&pExpr, CYPHER_EXPR_LITERAL);
    if (rc != SQLITE_OK) return rc;
    
    {
        CypherValue *pCopy = cypherValueCopy(pValue);
        if( pCopy ) {
            pExpr->u.literal = *pCopy;
            sqlite3_free(pCopy);
            rc = SQLITE_OK;
        } else {
            rc = SQLITE_NOMEM;
        }
    }
    if (rc != SQLITE_OK) {
        cypherExpressionDestroy(pExpr);
        return rc;
    }
    
    *ppExpr = pExpr;
    return SQLITE_OK;
}

/* Arithmetic expression creation */
int cypherExpressionCreateArithmetic(CypherExpression **ppExpr,
                                    CypherExpression *pLeft,
                                    CypherExpression *pRight,
                                    CypherArithmeticOp op) {
    CypherExpression *pExpr;
    int rc;
    
    if (!ppExpr || !pLeft || !pRight) return SQLITE_MISUSE;
    
    rc = cypherExpressionCreate(&pExpr, CYPHER_EXPR_ARITHMETIC);
    if (rc != SQLITE_OK) return rc;
    
    pExpr->u.binary.pLeft = pLeft;
    pExpr->u.binary.pRight = pRight;
    pExpr->u.binary.op = op;
    
    *ppExpr = pExpr;
    return SQLITE_OK;
}

/* Built-in function registration */
static CypherBuiltinFunction g_builtinFunctions[] = {
    {"toUpper", 1, 1, cypherFunctionToUpper},
    {"toLower", 1, 1, cypherFunctionToLower},
    {"length", 1, 1, cypherFunctionLength},
    {"size", 1, 1, cypherFunctionSize},
    {"abs", 1, 1, cypherFunctionAbs},
    {"ceil", 1, 1, cypherFunctionCeil},
    {"floor", 1, 1, cypherFunctionFloor},
    {"round", 1, 1, cypherFunctionRound},
    {"sqrt", 1, 1, cypherFunctionSqrt},
    {"toString", 1, 1, cypherFunctionToString},
    {NULL, 0, 0, NULL} /* Sentinel */
};

int cypherRegisterBuiltinFunctions(void) {
    g_functions = g_builtinFunctions;
    g_nFunctions = sizeof(g_builtinFunctions) / sizeof(g_builtinFunctions[0]) - 1;
    return SQLITE_OK;
}

const CypherBuiltinFunction *cypherGetBuiltinFunction(const char *zName) {
    int i;
    
    if (!zName) return NULL;
    
    for (i = 0; i < g_nFunctions; i++) {
        if (sqlite3_stricmp(zName, g_functions[i].zName) == 0) {
            return &g_functions[i];
        }
    }
    
    return NULL;
}

/* String functions implementation */
int cypherFunctionToUpper(CypherValue *apArgs, int nArgs, CypherValue *pResult) {
    const char *zInput;
    char *zOutput;
    int nLen, i;
    
    if (nArgs != 1 || !pResult) return SQLITE_MISUSE;
    
    if (apArgs[0].type == CYPHER_VALUE_NULL) {
        cypherValueSetNull(pResult);
        return SQLITE_OK;
    }
    
    if (apArgs[0].type != CYPHER_VALUE_STRING) {
        return SQLITE_MISMATCH;
    }
    
    zInput = apArgs[0].u.zString;
    nLen = strlen(zInput);
    
    zOutput = sqlite3_malloc(nLen + 1);
    if (!zOutput) return SQLITE_NOMEM;
    
    for (i = 0; i < nLen; i++) {
        zOutput[i] = toupper(zInput[i]);
    }
    zOutput[nLen] = '\0';
    
    cypherValueSetString(pResult, zOutput);
    sqlite3_free(zOutput);
    
    return SQLITE_OK;
}

int cypherFunctionToLower(CypherValue *apArgs, int nArgs, CypherValue *pResult) {
    const char *zInput;
    char *zOutput;
    int nLen, i;
    
    if (nArgs != 1 || !pResult) return SQLITE_MISUSE;
    
    if (apArgs[0].type == CYPHER_VALUE_NULL) {
        cypherValueSetNull(pResult);
        return SQLITE_OK;
    }
    
    if (apArgs[0].type != CYPHER_VALUE_STRING) {
        return SQLITE_MISMATCH;
    }
    
    zInput = apArgs[0].u.zString;
    nLen = strlen(zInput);
    
    zOutput = sqlite3_malloc(nLen + 1);
    if (!zOutput) return SQLITE_NOMEM;
    
    for (i = 0; i < nLen; i++) {
        zOutput[i] = tolower(zInput[i]);
    }
    zOutput[nLen] = '\0';
    
    cypherValueSetString(pResult, zOutput);
    sqlite3_free(zOutput);
    
    return SQLITE_OK;
}

int cypherFunctionLength(CypherValue *apArgs, int nArgs, CypherValue *pResult) {
    if (nArgs != 1 || !pResult) return SQLITE_MISUSE;
    
    if (apArgs[0].type == CYPHER_VALUE_NULL) {
        cypherValueSetNull(pResult);
        return SQLITE_OK;
    }
    
    if (apArgs[0].type == CYPHER_VALUE_STRING) {
        cypherValueSetInteger(pResult, strlen(apArgs[0].u.zString));
    } else {
        return SQLITE_MISMATCH;
    }
    
    return SQLITE_OK;
}

/* Math functions implementation */
int cypherFunctionAbs(CypherValue *apArgs, int nArgs, CypherValue *pResult) {
    if (nArgs != 1 || !pResult) return SQLITE_MISUSE;
    
    if (apArgs[0].type == CYPHER_VALUE_NULL) {
        cypherValueSetNull(pResult);
        return SQLITE_OK;
    }
    
    if (apArgs[0].type == CYPHER_VALUE_INTEGER) {
        sqlite3_int64 val = apArgs[0].u.iInteger;
        cypherValueSetInteger(pResult, val < 0 ? -val : val);
    } else if (apArgs[0].type == CYPHER_VALUE_FLOAT) {
        cypherValueSetFloat(pResult, fabs(apArgs[0].u.rFloat));
    } else {
        return SQLITE_MISMATCH;
    }
    
    return SQLITE_OK;
}

int cypherFunctionCeil(CypherValue *apArgs, int nArgs, CypherValue *pResult) {
    double val;
    
    if (nArgs != 1 || !pResult) return SQLITE_MISUSE;
    
    if (apArgs[0].type == CYPHER_VALUE_NULL) {
        cypherValueSetNull(pResult);
        return SQLITE_OK;
    }
    
    if (apArgs[0].type == CYPHER_VALUE_INTEGER) {
        cypherValueSetInteger(pResult, apArgs[0].u.iInteger);
    } else if (apArgs[0].type == CYPHER_VALUE_FLOAT) {
        val = ceil(apArgs[0].u.rFloat);
        cypherValueSetFloat(pResult, val);
    } else {
        return SQLITE_MISMATCH;
    }
    
    return SQLITE_OK;
}

int cypherFunctionSqrt(CypherValue *apArgs, int nArgs, CypherValue *pResult) {
    double val;
    
    if (nArgs != 1 || !pResult) return SQLITE_MISUSE;
    
    if (apArgs[0].type == CYPHER_VALUE_NULL) {
        cypherValueSetNull(pResult);
        return SQLITE_OK;
    }
    
    if (apArgs[0].type == CYPHER_VALUE_INTEGER) {
        val = sqrt((double)apArgs[0].u.iInteger);
    } else if (apArgs[0].type == CYPHER_VALUE_FLOAT) {
        val = sqrt(apArgs[0].u.rFloat);
    } else {
        return SQLITE_MISMATCH;
    }
    
    if (val < 0) {
        cypherValueSetNull(pResult); /* NaN for negative values */
    } else {
        cypherValueSetFloat(pResult, val);
    }
    
    return SQLITE_OK;
}

/* Additional function stubs */
int cypherFunctionFloor(CypherValue *apArgs, int nArgs, CypherValue *pResult) {
    /* Implementation similar to ceil */
    return cypherFunctionCeil(apArgs, nArgs, pResult); /* Placeholder */
}

int cypherFunctionRound(CypherValue *apArgs, int nArgs, CypherValue *pResult) {
    /* Implementation similar to ceil */
    return cypherFunctionCeil(apArgs, nArgs, pResult); /* Placeholder */
}

int cypherFunctionSize(CypherValue *apArgs, int nArgs, CypherValue *pResult) {
    /* Alias for length for now */
    return cypherFunctionLength(apArgs, nArgs, pResult);
}

int cypherFunctionToString(CypherValue *apArgs, int nArgs, CypherValue *pResult) {
    char *zStr;
    
    if (nArgs != 1 || !pResult) return SQLITE_MISUSE;
    
    zStr = cypherValueToString(&apArgs[0]);
    if (!zStr) return SQLITE_NOMEM;
    
    cypherValueSetString(pResult, zStr);
    sqlite3_free(zStr);
    
    return SQLITE_OK;
}