
#include "soltoken.h"
#include "solfunc.h"
#include "solop.h"
#include "soltypes.h"
#include "solerror.h"

static void inline sol_func_substitute_parameters(SolList parameters, SolList arguments, bool evaluate_tokens, bool evaluate_lists);

SolFunction sol_func_create(SolList parameters, SolList statements) {
    return (SolFunction) sol_obj_clone_type((SolObject) Function, &(struct sol_func_raw){
            false,
            (SolList) sol_obj_retain((SolObject) parameters),
            (SolList) sol_obj_retain((SolObject) statements),
            sol_token_pool_snapshot()
        }, sizeof(sol_func));
}

SolObject sol_func_execute(SolFunction func, SolList arguments, SolObject self) {
    bool evaluate_tokens = sol_bool_value_of(sol_obj_get_prop((SolObject) func, "$evaluate-tokens"))->value;
    bool evaluate_lists = sol_bool_value_of(sol_obj_get_prop((SolObject) func, "$evaluate-lists"))->value;
    
    if (func->is_operator) {
        SolList evaluated = (SolList) sol_obj_retain((SolObject) sol_list_create(false));
        SOL_LIST_ITR(arguments, current, i) {
            SolObject object = current->value;
            switch (object->type_id) {
                case TYPE_SOL_TOKEN:
                    if (evaluate_tokens) {
                        SolObject evaluated_object = sol_obj_evaluate(object);
                        sol_list_add_obj(evaluated, evaluated_object);
                        sol_obj_release(evaluated_object);
                    } else {
                        sol_list_add_obj(evaluated, object);
                    }
                    break;
                case TYPE_SOL_LIST:
                    if (evaluate_lists) {
                        SolObject evaluated_object = sol_obj_evaluate(object);
                        sol_list_add_obj(evaluated, evaluated_object);
                        sol_obj_release(evaluated_object);
                    } else {
                        sol_list_add_obj(evaluated, object);
                    }
                    break;
                default: {
                    SolObject evaluated_object = sol_obj_evaluate(object);
                    sol_list_add_obj(evaluated, evaluated_object);
                    sol_obj_release(evaluated_object);
                }
            }
        }
        SolObject result = ((SolOperator) func)->operator_ref(evaluated, self);
        sol_obj_release((SolObject) evaluated);
        return result;
    }
    
    SolList statements = func->statements;
    SolObject ans = nil;
    
    try {
        // establish closure scope
        sol_token_pool_push_m(func->closure_scope);
        // create function scope
        sol_token_pool_push();
        
        // create 'this' reference
        sol_token_register("this", (SolObject) func);
        // create 'self' reference
        sol_token_register("self", self);
        
        // insert 'arguments' reference and perform parameter substitution
        sol_func_substitute_parameters(func->parameters, arguments, evaluate_tokens, evaluate_lists);
        
        // execute function statements
        SOL_LIST_ITR(statements, current, i) {
            sol_obj_release(ans);
            ans = sol_obj_evaluate(current->value);
        }
    } finally {
        // destroy function/closure scope
        sol_token_pool_pop();
        func->closure_scope = sol_token_pool_pop_m();
    }
    
    return ans;
}

static void inline sol_func_substitute_parameters(SolList parameters, SolList arguments, bool evaluate_tokens, bool evaluate_lists) {
    // evaluate arguments
    SolList evaluated = sol_list_create(false);
    sol_token_register("arguments", (SolObject) evaluated);
    SOL_LIST_ITR(arguments, current, i) {
        SolObject object = current->value;
        switch (object->type_id) {
            case TYPE_SOL_TOKEN:
                if (evaluate_tokens) {
                    SolObject evaluated_object = sol_obj_evaluate(object);
                    sol_list_add_obj(evaluated, evaluated_object);
                    sol_obj_release(evaluated_object);
                } else {
                    sol_list_add_obj(evaluated, object);
                }
                break;
            case TYPE_SOL_LIST:
                if (evaluate_lists) {
                    SolObject evaluated_object = sol_obj_evaluate(object);
                    sol_list_add_obj(evaluated, evaluated_object);
                    sol_obj_release(evaluated_object);
                } else {
                    sol_list_add_obj(evaluated, object);
                }
                break;
            default: {
                SolObject evaluated_object = sol_obj_evaluate(object);
                sol_list_add_obj(evaluated, object);
                sol_obj_release(evaluated_object);
            }
        }
    }
    
    // register parameters
    SOL_LIST_ITR_PARALLEL(parameters, current_parameter, i_parameter, evaluated, current_evaluated, i_evaluated) {
        if (!current_parameter) break;
        SolToken token = (SolToken) current_parameter->value;
        SolObject object = current_evaluated ? current_evaluated->value : nil;
        sol_token_register(token->identifier, object);
    }
}
