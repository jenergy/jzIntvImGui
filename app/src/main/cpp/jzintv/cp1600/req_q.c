#include "config.h"
#include "cp1600/req_q.h"


/* ======================================================================== */
/*  REQ_Q_CLEAR      -- Empty the request queue.                            */
/* ======================================================================== */
void req_q_clear(req_q_t *const RESTRICT req_q)
{
    memset(req_q->req, 0, sizeof(req_q->req));
    req_q->wr      = 0;
    req_q->rd      = 0;
    req_q->horizon = ~0ull;
} 

/* ======================================================================== */
/*  REQ_Q_PRINT      -- Print out the contents of a req_q for debugging.    */
/* ======================================================================== */
void req_q_print(req_q_t    *const RESTRICT rq,
                 const char *const RESTRICT context)
{
    const int mrd = rq->rd & (REQ_Q_DEPTH - 1); 
    const int mwr = rq->wr & (REQ_Q_DEPTH - 1); 
    int i; 

    jzp_printf("REQ_Q(%s): rd=%02X wr=%02X horizon=%9" LL_FMT "u\n",
                context ? context : "",
                rq->rd, rq->wr, (unsigned long long)rq->horizon);

    jzp_printf("REQ# R W %9s %9s %9s %5s %s\n",
                "START", "END", "ACK CYC", "TYPE", "STATUS");
    for (i = 0; i < REQ_Q_DEPTH; i++)
    {
        const req_t *const RESTRICT req = &rq->req[i];
        jzp_printf
        (
            "%03d: %c %c %9" LL_FMT "u %9" LL_FMT "u %9" LL_FMT "u %s %s\n",
            i, i == mrd ? 'r' : ' ', i == mwr ? 'w' : ' ',
            (unsigned long long)req->start,
            (unsigned long long)req->end,
            (unsigned long long)req->ack_cycle, 
            req->type == REQ_INT ? "INTRQ" :
            req->type == REQ_BUS ? "BUSRQ" : "<?-?>",
            req->state == REQ_INACTIVE ? "INACTIVE" :
            req->state == REQ_ACKED    ? "ACKED   " :
            req->state == REQ_DROPPED  ? "DROPPED " :
            req->state == REQ_PENDING  ? "PENDING " : "<?----?>"
        );
    }
}

/* ======================================================================== */
/*  REQ_Q_DEFAULT_ACK_FN                                                    */
/*  Default request queue ACK function just marks request ACKed.            */
/* ======================================================================== */
void req_q_default_ack_fn
(
    struct req_q_t *const RESTRICT  req_q,
    const uint64_t                  cycle
)
{
    req_t *const RESTRICT req = &REQ_Q_FRONT(req_q);
    req->ack_cycle = cycle;
    req->state     = REQ_ACKED;
    return;
}

/* ======================================================================== */
/*  REQ_Q_DEFAULT_DROP_FN                                                   */
/*  Default request queue DROP function just marks request DROPped.         */
/* ======================================================================== */
void req_q_default_drop_fn
(
    struct req_q_t *const RESTRICT  req_q,
    const uint64_t                  cycle
)
{
    req_t *const RESTRICT req = &REQ_Q_FRONT(req_q);
    req->ack_cycle = cycle;
    req->state     = REQ_DROPPED;
    return;
}

/* ======================================================================== */
/*  REQ_Q_INIT   -- Initialize a request queue.                             */
/* ======================================================================== */
void req_q_init(req_q_t *const RESTRICT req_q)
{
    req_q_clear(req_q);
    req_q->ack  = req_q_default_ack_fn;
    req_q->drop = req_q_default_drop_fn;
}

/* ======================================================================== */
/*  This program is free software; you can redistribute it and/or modify    */
/*  it under the terms of the GNU General Public License as published by    */
/*  the Free Software Foundation; either version 2 of the License, or       */
/*  (at your option) any later version.                                     */
/*                                                                          */
/*  This program is distributed in the hope that it will be useful,         */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       */
/*  General Public License for more details.                                */
/*                                                                          */
/*  You should have received a copy of the GNU General Public License along */
/*  with this program; if not, write to the Free Software Foundation, Inc., */
/*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             */
/* ======================================================================== */
/*                   Copyright (c) 2017, Joseph Zbiciak                     */
/* ======================================================================== */
