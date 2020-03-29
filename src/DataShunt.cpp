#include "DataShunt.h"

void DataShunt::init(const Eigen::VectorXd & shunt_p_mw,
                     const Eigen::VectorXd & shunt_q_mvar,
                     const Eigen::VectorXi & shunt_bus_id)
{
    p_mw_ = shunt_p_mw;
    q_mvar_ = shunt_q_mvar;
    bus_id_ = shunt_bus_id;
    status_ = std::vector<bool>(p_mw_.size(), true); // by default everything is connected
}

void DataShunt::fillYbus(Eigen::SparseMatrix<cdouble> & res, bool ac, const std::vector<int> & id_grid_to_solver){
    int nb_shunt = q_mvar_.size();
    cdouble tmp;
    int bus_id_me, bus_id_solver;
    for(int shunt_id=0; shunt_id < nb_shunt; ++shunt_id){
        // i don't do anything if the shunt is disconnected
        if(!status_[shunt_id]) continue;

        // assign diagonal coefficient
        tmp = p_mw_(shunt_id) + 1.0i * q_mvar_(shunt_id);
        bus_id_me = bus_id_(shunt_id);
        bus_id_solver = id_grid_to_solver[bus_id_me];
        if(bus_id_solver == _deactivated_bus_id){
            throw std::runtime_error("GridModel::fillYbusShunt: A shunt is connected to a disconnected bus.");
        }
        res.coeffRef(bus_id_solver, bus_id_solver) -= tmp;
    }
}

void DataShunt::compute_results(const Eigen::Ref<Eigen::VectorXd> & Va,
                               const Eigen::Ref<Eigen::VectorXd> & Vm,
                               const Eigen::Ref<Eigen::VectorXcd> & V,
                               const std::vector<int> & id_grid_to_solver,
                               const Eigen::VectorXd & bus_vn_kv)
{
    int nb_shunt = p_mw_.size();
    v_kv_from_vpu(Va, Vm, status_, nb_shunt, bus_id_, id_grid_to_solver, bus_vn_kv, res_v_);
    res_p_ = Eigen::VectorXd::Constant(nb_shunt, 0.);
    res_q_ = Eigen::VectorXd::Constant(nb_shunt, 0.);
    const cdouble my_i = 1.0i;
    for(int shunt_id = 0; shunt_id < nb_shunt; ++shunt_id){
        if(!status_[shunt_id]) continue;
        int bus_id_me = bus_id_(shunt_id);
        int bus_solver_id = id_grid_to_solver[bus_id_me];
        if(bus_solver_id == _deactivated_bus_id){
            throw std::runtime_error("DataShunt::compute_results: A shunt is connected to a disconnected bus.");
        }
        cdouble E = V(bus_solver_id);
        cdouble y = -1.0 * (p_mw_(shunt_id) + my_i * q_mvar_(shunt_id));
        cdouble I = y * E;
        I = std::conj(I);
        cdouble s = E * I;
        res_p_(shunt_id) = std::real(s);
        res_q_(shunt_id) = std::imag(s);
    }
}

void DataShunt::reset_results(){
    res_p_ = Eigen::VectorXd();  // in MW
    res_q_ = Eigen::VectorXd();  // in MVar
    res_v_ = Eigen::VectorXd();  // in kV
}