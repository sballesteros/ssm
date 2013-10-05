/**************************************************************************
 *    This file is part of ssm.
 *
 *    ssm is free software: you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published
 *    by the Free Software Foundation, either version 3 of the
 *    License, or (at your option) any later version.
 *
 *    ssm is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public
 *    License along with ssm.  If not, see
 *    <http://www.gnu.org/licenses/>.
 *************************************************************************/

#include "ssm.h"

int main(int argc, char *argv[])
{
    ssm_options_t *opts = ssm_options_new();
    ssm_load_options(opts, SSM_PMCMC, argc, argv);

    json_t *jparameters = ssm_load_json_stream(stdin);
    json_t *jdata = ssm_load_data(opts);

    ssm_nav_t *nav = ssm_nav_new(jparameters, opts);
    ssm_data_t *data = ssm_data_new(jdata, nav, opts);
    ssm_fitness_t *fitness = ssm_fitness_new(data, opts);
    ssm_calc_t **calc = ssm_N_calc_new(jdata, nav, data, fitness, opts);
    ssm_X_t ***D_J_X = ssm_D_J_X_new(data, fitness, nav, opts);
    ssm_X_t ***D_J_X_tmp = ssm_D_J_X_new(data, fitness, nav, opts);

    json_decref(jdata);

    ssm_input_t *input = ssm_input_new(jparameters, nav);
    ssm_par_t *par = ssm_par_new(input, calc[0], nav);

    ssm_theta_t *theta = ssm_theta_new(nav);
    ssm_theta_t *proposed = ssm_theta_new(nav);
    ssm_var_t *var_input = ssm_var_new(jparameters, nav);
    ssm_var_t *var; //the covariance matrix used;
    ssm_adapt_t *adapt = ssm_adapt_new(nav, opts);

    int n_iter = opts->n_iter;

    /////////////////////////
    // initialization step //
    /////////////////////////
    int m = 0;

    ssm_input2par(par, input, calc[0], nav);
    ssm_par2X(D_J_X[0][0], par, calc[0], nav);
    for(j=1; j<fitness->J; j++){
        ssm_X_copy(D_J_X[0][j], D_J_X[0][0]);
    }

    //TODO: RUN SMC

    fitness->log_like_new = p_like->log_like;

    //TODO SAMPLE TRAJ

    //the initial iteration is "accepted"
    fitness->log_like_prev = fitness->log_like_new;

    ////////////////
    // iterations //
    ////////////////
    double sd_fac;
    int is_accepted;
    double ratio;
    for(m=1; m<n_iter; m++) {
        // generate new theta	
	var = ssm_adapt_eps_var_sd_fac(&sd_fac, adapt, var_input, nav, m);
	ssm_theta_ran(proposed, theta, var, sd_fac, calc, nav, 1);
	ssm_theta2input(input, proposed, nav);
	ssm_input2par(par, input, calc, nav);
	ssm_par2X(D_J_X[0][0], par, calc[0], nav);
	for(j=1; j<fitness->J; j++){
	    ssm_X_copy(D_J_X[0][j], D_J_X[0][0]);
	}

	//TODO: RUN SMC

	fitness->log_like_new = p_like->log_like;

	is_accepted =  ssm_metropolis_hastings(&ratio, proposed, theta, var, sd_fac, fitness, nav, calc, 1);

        if (is_accepted) {
	    fitness->log_like_prev = fitness->log_like_new;
	    ssm_theta_copy(theta, proposed);
	}
 
	ssm_adapt_ar(adapt, is_accepted, m); //compute acceptance rate

	ssm_adapt_var(adapt, theta, m);  //compute empirical variance
    }


    json_decref(jparameters);

    ssm_D_J_X_free(D_J_X, data, fitness);
    ssm_N_calc_free(calc, nav);

    ssm_data_free(data);
    ssm_nav_free(nav);

    ssm_fitness_free(fitness);

    ssm_input_free(input);
    ssm_par_free(par);

    ssm_theta_free(theta);
    ssm_theta_free(proposed);
    ssm_var_free(var_input);
    ssm_adapt_free(adapt);

    return 0;
}