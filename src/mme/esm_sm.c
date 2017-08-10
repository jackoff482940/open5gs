#define TRACE_MODULE _esm_sm

#include "core_debug.h"

#include "nas_message.h"

#include "mme_event.h"
#include "mme_s6a_handler.h"
#include "emm_handler.h"
#include "esm_build.h"
#include "esm_handler.h"
#include "mme_s11_build.h"
#include "mme_s11_path.h"

void esm_state_initial(fsm_t *s, event_t *e)
{
    d_assert(s, return, "Null param");

    mme_sm_trace(3, e);

    FSM_TRAN(s, &esm_state_operational);
}

void esm_state_final(fsm_t *s, event_t *e)
{
    d_assert(s, return, "Null param");

    mme_sm_trace(3, e);
}

void esm_state_operational(fsm_t *s, event_t *e)
{
    d_assert(s, return, "Null param");
    d_assert(e, return, "Null param");

    mme_sm_trace(3, e);

    switch (event_get(e))
    {
        case FSM_ENTRY_SIG:
        {
            break;
        }
        case FSM_EXIT_SIG:
        {
            break;
        }
        case MME_EVT_ESM_BEARER_MSG:
        {
            index_t index = event_get_param1(e);
            mme_bearer_t *bearer = NULL;
            mme_ue_t *mme_ue = NULL;
            nas_message_t *message = NULL;

            d_assert(index, return, "Null param");
            bearer = mme_bearer_find(index);
            d_assert(bearer, return, "Null param");
            mme_ue = bearer->mme_ue;
            d_assert(mme_ue, return, "Null param");
            message = (nas_message_t *)event_get_param4(e);
            d_assert(message, break, "Null param");

            /* Save Last Received NAS-ESM message */
            memcpy(&mme_ue->last_esm_message, message, sizeof(nas_message_t));

            switch(message->esm.h.message_type)
            {
                case NAS_PDN_CONNECTIVITY_REQUEST:
                {
                    esm_handle_pdn_connectivity_request(
                            bearer, &message->esm.pdn_connectivity_request);
                    d_trace(3, "[NAS] PDN connectivity request : "
                            "UE[%s] --> ESM[%d]\n", 
                            mme_ue->imsi_bcd, bearer->pti);

                    if (MME_UE_HAVE_IMSI(mme_ue))
                    {
                        /* Known GUTI */
                        if (SECURITY_CONTEXT_IS_VALID(mme_ue))
                        {
                            mme_sess_t *sess = bearer->sess;
                            d_assert(sess, return, "Null param");

                            if (MME_SESSION_HAVE_APN(sess))
                            {
                                status_t rv;
                                pkbuf_t *pkbuf = NULL;

                                if (MME_SESSION_IS_CREATED(mme_ue))
                                {
                                    emm_handle_attach_accept(mme_ue);
                                }
                                else
                                {
                                    rv = mme_s11_build_create_session_request(
                                            &pkbuf, bearer);
                                    d_assert(rv == CORE_OK, return,
                                            "S11 build error");

                                    rv = mme_s11_send_to_sgw(bearer->sgw, 
                                            GTP_CREATE_SESSION_REQUEST_TYPE,
                                            0, pkbuf);
                                    d_assert(rv == CORE_OK, return,
                                            "S11 send error");
                                }
                            }
                            else
                            {
                                esm_handle_information_request(mme_ue);
                            }
                        }
                        else
                        {
                            if (MME_SESSION_IS_CREATED(mme_ue))
                            {
                                emm_handle_s11_delete_session_request(mme_ue);
                            }
                            else
                            {
                                mme_s6a_send_air(mme_ue);
                            }
                        }
                    }
                    else
                    {
                        /* Continue with Identity Response */
                    }
                    break;
                }
                case NAS_ESM_INFORMATION_RESPONSE:
                {
                    /* FIXME : SGW Selection */
                    bearer->sgw = mme_sgw_first();

                    d_trace(3, "[NAS] ESM information response : "
                            "UE[%s] --> ESM[%d]\n", 
                            mme_ue->imsi_bcd, bearer->pti);
                    esm_handle_information_response(
                            bearer, &message->esm.esm_information_response);
                    break;
                }
                case NAS_ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
                {
                    d_trace(3, "[NAS] Activate default eps bearer "
                            "context accept : UE[%s] --> ESM[%d]\n", 
                            mme_ue->imsi_bcd, bearer->pti);
                    break;
                }
                default:
                {
                    d_warn("Not implemented(type:%d)", 
                            message->esm.h.message_type);
                    break;
                }
            }
            break;
        }

        default:
        {
            d_error("Unknown event %s", mme_event_get_name(e));
            break;
        }
    }
}

void esm_state_exception(fsm_t *s, event_t *e)
{
    d_assert(s, return, "Null param");
    d_assert(e, return, "Null param");

    mme_sm_trace(3, e);

    switch (event_get(e))
    {
        case FSM_ENTRY_SIG:
        {
            break;
        }
        case FSM_EXIT_SIG:
        {
            break;
        }
        default:
        {
            d_error("Unknown event %s", mme_event_get_name(e));
            break;
        }
    }
}
