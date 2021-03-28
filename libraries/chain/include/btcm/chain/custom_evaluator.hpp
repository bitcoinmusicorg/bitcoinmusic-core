#pragma once
#include <btcm/chain/protocol/operations.hpp>
#include <btcm/chain/evaluator.hpp>
#include <btcm/chain/database.hpp>

namespace btcm { namespace chain {

   class custom_evaluator : public evaluator<custom_evaluator>
   {
      public:
         typedef custom_operation operation_type;

         void do_evaluate( const custom_operation& o ){}
         void do_apply( const custom_operation& o ){}
   };
} }
